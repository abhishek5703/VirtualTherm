#include "virtualtherm/csv_logger.hpp"
#include "virtualtherm/fan_controller.hpp"
#include "virtualtherm/scenario.hpp"
#include "virtualtherm/temperature_filter.hpp"
#include "virtualtherm/temperature_simulator.hpp"
#include "virtualtherm/virtual_sensor.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

namespace {

using virtualtherm::ControllerStatus;
using virtualtherm::CsvLogger;
using virtualtherm::FanMode;
using virtualtherm::SimulationSample;

struct Options {
    int steps{150};
    virtualtherm::Scenario scenario{virtualtherm::Scenario::Normal};
    std::string csvPath{"virtualtherm_output.csv"};
    bool realtime{false};
    bool quiet{false};
};

struct RunStats {
    int fanOff{0};
    int fanLow{0};
    int fanMedium{0};
    int fanHigh{0};
    int normalSamples{0};
    int warningSamples{0};
    int criticalSamples{0};
    int sensorFaultSamples{0};
    int sensorInvalidSamples{0};
    int events{0};
    int fanTransitions{0};
    double sumRawTemp{0.0};
    int validTempSamples{0};
    double peakPlantTemp{-std::numeric_limits<double>::infinity()};
    double peakRawTemp{-std::numeric_limits<double>::infinity()};
    double sumFanPercent{0.0};

    void observe(const SimulationSample& sample, FanMode previousMode) {
        switch (sample.fanMode) {
            case FanMode::Off: ++fanOff; break;
            case FanMode::Low: ++fanLow; break;
            case FanMode::Medium: ++fanMedium; break;
            case FanMode::High: ++fanHigh; break;
        }
        switch (sample.status) {
            case ControllerStatus::Normal: ++normalSamples; break;
            case ControllerStatus::Warning: ++warningSamples; break;
            case ControllerStatus::Critical: ++criticalSamples; break;
            case ControllerStatus::SensorFault: ++sensorFaultSamples; break;
        }
        if (!sample.sensorValid) {
            ++sensorInvalidSamples;
        } else {
            sumRawTemp += sample.rawSensorTemperatureC;
            ++validTempSamples;
            peakRawTemp = std::max(peakRawTemp, sample.rawSensorTemperatureC);
        }
        peakPlantTemp = std::max(peakPlantTemp, sample.plantTemperatureC);
        sumFanPercent += sample.fanPercent;
        if (!sample.event.empty()) ++events;
        if (sample.fanMode != previousMode) ++fanTransitions;
    }
};

void printUsage(const char* executable) {
    std::cout
        << "VirtualTherm - software-only embedded cooling controller simulation\n\n"
        << "Usage: " << executable << " [options]\n\n"
        << "Options:\n"
        << "  --scenario NAME   normal | overheat | sensor-fault | workload-spike | noisy-sensor\n"
        << "  --steps N         Number of simulated seconds, 1..36000 (default: 150)\n"
        << "  --csv PATH        CSV output path (default: virtualtherm_output.csv)\n"
        << "  --realtime        Wait one real second per simulated second\n"
        << "  --quiet           Print only the final summary\n"
        << "  --help            Show this help message\n\n"
        << "Examples:\n"
        << "  " << executable << " --scenario normal --steps 150\n"
        << "  " << executable << " --scenario overheat --csv overheat.csv\n"
        << "  " << executable << " --scenario sensor-fault --steps 90\n"
        << "  " << executable << " --scenario workload-spike\n"
        << "  " << executable << " --scenario noisy-sensor --csv noisy.csv\n";
}

bool parsePositiveInt(const std::string& value, int& result) {
    try {
        std::size_t charsRead = 0;
        const long parsed = std::stol(value, &charsRead);
        if (charsRead != value.size() || parsed < 1 || parsed > 36000) return false;
        result = static_cast<int>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseArgs(int argc, char* argv[], Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return false;
        }
        if (arg == "--realtime") { options.realtime = true; continue; }
        if (arg == "--quiet") { options.quiet = true; continue; }
        if (arg == "--scenario" || arg == "--steps" || arg == "--csv") {
            if (i + 1 >= argc) {
                std::cerr << "Error: missing value after " << arg << ".\n";
                return false;
            }
            const std::string value = argv[++i];
            if (arg == "--scenario") {
                const auto parsed = virtualtherm::SimulationScenario::parse(value);
                if (!parsed) {
                    std::cerr << "Error: unknown scenario '" << value << "'. Use --help for available scenarios.\n";
                    return false;
                }
                options.scenario = *parsed;
            } else if (arg == "--steps") {
                if (!parsePositiveInt(value, options.steps)) {
                    std::cerr << "Error: --steps must be an integer from 1 to 36000.\n";
                    return false;
                }
            } else {
                options.csvPath = value;
                if (options.csvPath.empty()) {
                    std::cerr << "Error: --csv path cannot be empty.\n";
                    return false;
                }
            }
            continue;
        }
        std::cerr << "Error: unknown option '" << arg << "'. Use --help for usage.\n";
        return false;
    }
    return true;
}

void printHeader() {
    std::cout << std::left
              << std::setw(6) << "TIME"
              << std::setw(9) << "LOAD"
              << std::setw(10) << "RAW C"
              << std::setw(10) << "FILTER C"
              << std::setw(9) << "FAN"
              << std::setw(8) << "RPM*"
              << std::setw(14) << "STATUS"
              << "EVENT\n";
    std::cout << std::string(91, '-') << '\n';
}

void printSample(const SimulationSample& sample) {
    std::cout << std::left << std::setw(6) << sample.timeSeconds
              << std::setw(9) << std::fixed << std::setprecision(2) << sample.workload;
    if (sample.sensorValid) {
        std::cout << std::setw(10) << sample.rawSensorTemperatureC
                  << std::setw(10) << sample.filteredTemperatureC;
    } else {
        std::cout << std::setw(10) << "INVALID" << std::setw(10) << "--";
    }
    std::cout << std::setw(9) << (std::string(virtualtherm::toString(sample.fanMode)) + "/" + std::to_string(sample.fanPercent))
              << std::setw(8) << sample.fanRpmEstimate
              << std::setw(14) << virtualtherm::toString(sample.status)
              << sample.event << '\n';
}

void printSummary(const Options& options, const virtualtherm::SimulationScenario& scenario,
                  const virtualtherm::TemperatureSimulator& simulator, const virtualtherm::FanController& controller,
                  const RunStats& stats, const CsvLogger& logger) {
    const double averageTemp = stats.validTempSamples > 0 ? stats.sumRawTemp / stats.validTempSamples : 0.0;
    const double averageFan = options.steps > 0 ? stats.sumFanPercent / options.steps : 0.0;
    std::cout << "\n================ RUN SUMMARY ================\n"
              << "Scenario:                 " << scenario.name() << '\n'
              << "Samples simulated:        " << options.steps << '\n'
              << "Final plant temperature:  " << std::fixed << std::setprecision(2) << simulator.temperatureC() << " C\n"
              << "Peak plant temperature:   " << stats.peakPlantTemp << " C\n";
    if (stats.validTempSamples > 0) {
        std::cout << "Peak raw sensor reading:  " << stats.peakRawTemp << " C\n"
                  << "Average raw temperature:  " << averageTemp << " C\n";
    } else {
        std::cout << "Peak raw sensor reading:  unavailable (no valid readings)\n";
    }
    std::cout << "Final controller status:  " << virtualtherm::toString(controller.status()) << '\n'
              << "Final fan command:        " << virtualtherm::toString(controller.fanMode())
              << " (" << virtualtherm::fanPercent(controller.fanMode()) << "%, modelled "
              << virtualtherm::estimatedFanRpm(controller.fanMode()) << " RPM)\n"
              << "Average fan command:      " << averageFan << "%\n"
              << "Fan mode duration:        OFF " << stats.fanOff << "s | LOW " << stats.fanLow
              << "s | MEDIUM " << stats.fanMedium << "s | HIGH " << stats.fanHigh << "s\n"
              << "Status samples:           NORMAL " << stats.normalSamples << " | WARNING " << stats.warningSamples
              << " | CRITICAL " << stats.criticalSamples << " | SENSOR_FAULT " << stats.sensorFaultSamples << '\n'
              << "Invalid sensor samples:   " << stats.sensorInvalidSamples << '\n'
              << "Fan mode transitions:     " << stats.fanTransitions << '\n'
              << "Logged event rows:        " << stats.events << '\n'
              << "CSV file:                 " << logger.path() << '\n'
              << "==============================================\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    Options options;
    if (!parseArgs(argc, argv, options)) return 2;

    virtualtherm::SimulationScenario scenario(options.scenario);
    virtualtherm::TemperatureSimulator simulator;
    virtualtherm::VirtualTemperatureSensor sensor;
    virtualtherm::TemperatureFilter filter(0.35);
    virtualtherm::FanController controller;
    virtualtherm::CsvLogger logger(options.csvPath);

    if (!logger.isOpen()) {
        std::cerr << "Error: could not open CSV output path '" << options.csvPath
                  << "'. Check that its directory exists and is writable.\n";
        return 1;
    }

    if (!options.quiet) {
        std::cout << "VirtualTherm | scenario=" << scenario.name()
                  << " | steps=" << options.steps
                  << " | mode=" << (options.realtime ? "paced simulation" : "fast simulation") << '\n'
                  << "CSV output: " << logger.path() << "\n"
                  << "* RPM is an estimate generated by the model, not measured hardware data.\n\n";
        printHeader();
    }

    RunStats stats;
    for (int t = 0; t < options.steps; ++t) {
        const double workload = scenario.workloadAt(t);
        const double plantTemperature = simulator.temperatureC();
        const virtualtherm::SensorReading reading = sensor.read(plantTemperature, t, scenario);
        const double filteredTemperature = filter.update(reading);
        const FanMode previousFanMode = controller.fanMode();
        const virtualtherm::ControllerOutput output = controller.update(reading, filteredTemperature);

        SimulationSample sample;
        sample.timeSeconds = t;
        sample.scenario = scenario.name();
        sample.workload = workload;
        sample.plantTemperatureC = plantTemperature;
        sample.sensorValid = reading.valid && std::isfinite(filteredTemperature);
        sample.rawSensorTemperatureC = reading.temperatureC;
        sample.filteredTemperatureC = filteredTemperature;
        sample.fanMode = output.fanMode;
        sample.fanPercent = virtualtherm::fanPercent(output.fanMode);
        sample.fanRpmEstimate = virtualtherm::estimatedFanRpm(output.fanMode);
        sample.status = output.status;
        sample.event = output.event;

        if (!logger.write(sample)) {
            std::cerr << "Error: failed while writing CSV output.\n";
            return 1;
        }
        stats.observe(sample, previousFanMode);

        const bool periodicPrint = (t % 5 == 0) || (t == options.steps - 1);
        if (!options.quiet && (periodicPrint || !output.event.empty())) printSample(sample);

        simulator.advance(workload, output.fanMode, 1.0);
        if (options.realtime) std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    printSummary(options, scenario, simulator, controller, stats, logger);
    return 0;
}
