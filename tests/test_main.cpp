#include "virtualtherm/fan_controller.hpp"
#include "virtualtherm/scenario.hpp"
#include "virtualtherm/temperature_filter.hpp"
#include "virtualtherm/temperature_simulator.hpp"
#include "virtualtherm/virtual_sensor.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (condition) std::cout << "[PASS] " << message << '\n';
    else { std::cerr << "[FAIL] " << message << '\n'; ++failures; }
}

void testFanThresholdsAndHysteresis() {
    using namespace virtualtherm;
    FanController controller;
    check(controller.update(SensorReading{true, 25.0, {}}, 25.0).fanMode == FanMode::Off,
          "fan stays OFF at room temperature");
    check(controller.update(SensorReading{true, 41.0, {}}, 41.0).fanMode == FanMode::Low,
          "fan enters LOW above the activation threshold");
    check(controller.update(SensorReading{true, 38.0, {}}, 38.0).fanMode == FanMode::Low,
          "hysteresis keeps fan LOW between thresholds");
    check(controller.update(SensorReading{true, 36.0, {}}, 36.0).fanMode == FanMode::Off,
          "fan returns OFF below the lower threshold");
    check(estimatedFanRpm(FanMode::High) == 2000, "fan RPM mapping is available to diagnostics");
}

void testWarningAndCriticalLatch() {
    using namespace virtualtherm;
    FanController controller;
    auto output = controller.update(SensorReading{true, 56.0, {}}, 56.0);
    check(output.fanMode == FanMode::Medium, "fan enters MEDIUM at elevated temperature");
    check(output.status == ControllerStatus::Warning, "elevated temperature raises WARNING status");
    output = controller.update(SensorReading{true, 86.0, {}}, 86.0);
    check(output.status == ControllerStatus::Critical && output.fanMode == FanMode::High,
          "critical temperature latches CRITICAL and commands HIGH fan");
    FanController rawOverrideController;
    output = rawOverrideController.update(SensorReading{true, 80.0, {}}, 65.0);
    check(output.status == ControllerStatus::Critical && output.fanMode == FanMode::High,
          "raw critical reading overrides a lagging filtered temperature");
    output = controller.update(SensorReading{true, 80.0, {}}, 80.0);
    check(output.status == ControllerStatus::Critical,
          "critical state remains latched until temperature falls below recovery threshold");
    output = controller.update(SensorReading{true, 70.0, {}}, 70.0);
    check(output.status == ControllerStatus::Warning,
          "critical latch clears only after temperature has cooled sufficiently");
}

void testSensorFaultRecoveryDebounce() {
    using namespace virtualtherm;
    FanController controller;
    const auto failed = controller.update(SensorReading{false, std::numeric_limits<double>::quiet_NaN(), "TEST"}, NAN);
    check(failed.fanMode == FanMode::High, "invalid sensor reading commands HIGH fan");
    check(failed.status == ControllerStatus::SensorFault, "invalid reading sets SENSOR_FAULT status");
    const auto recovery1 = controller.update(SensorReading{true, 30.0, {}}, 30.0);
    check(recovery1.status == ControllerStatus::SensorFault && recovery1.fanMode == FanMode::High,
          "first valid reading does not immediately clear sensor fault");
    const auto recovery2 = controller.update(SensorReading{true, 30.0, {}}, 30.0);
    check(recovery2.status == ControllerStatus::SensorFault && recovery2.fanMode == FanMode::High,
          "second valid reading keeps safe fan command active");
    const auto recovered = controller.update(SensorReading{true, 30.0, {}}, 30.0);
    check(recovered.status == ControllerStatus::Normal, "third consecutive valid reading clears sensor fault");
    check(recovered.fanMode == FanMode::Off, "controller returns to temperature-based control after recovery");
}

void testFilter() {
    using namespace virtualtherm;
    TemperatureFilter filter(0.5);
    check(std::abs(filter.update(SensorReading{true, 20.0, {}}) - 20.0) < 1e-9,
          "filter initializes from the first sample");
    check(std::abs(filter.update(SensorReading{true, 30.0, {}}) - 25.0) < 1e-9,
          "exponential moving average smooths a step change");
    check(std::isnan(filter.update(SensorReading{false, std::numeric_limits<double>::quiet_NaN(), "TEST"})),
          "invalid input resets filter instead of reusing stale data");
    check(std::abs(filter.update(SensorReading{true, 40.0, {}}) - 40.0) < 1e-9,
          "filter reinitializes on first sample after recovery");
}

void testScenarioProfilesAndParsing() {
    using namespace virtualtherm;
    const auto parsed = SimulationScenario::parse("sensor-fault");
    check(parsed.has_value() && *parsed == Scenario::SensorFault, "scenario parser accepts sensor-fault");
    check(SimulationScenario::parse("workload-spike").has_value(), "scenario parser accepts workload-spike");
    check(SimulationScenario::parse("noisy-sensor").has_value(), "scenario parser accepts noisy-sensor");
    check(!SimulationScenario::parse("imaginary").has_value(), "scenario parser rejects unknown scenario");
    SimulationScenario faultScenario(Scenario::SensorFault);
    check(faultScenario.sensorValidAt(34), "sensor is valid before injected fault");
    check(!faultScenario.sensorValidAt(35), "sensor fault begins at second 35");
    check(!faultScenario.sensorValidAt(44), "sensor remains invalid through second 44");
    check(faultScenario.sensorValidAt(45), "sensor recovers at second 45");

    SimulationScenario noiseScenario(Scenario::NoisySensor);
    check(std::abs(noiseScenario.sensorNoiseAt(10)) > 0.0, "noisy sensor scenario injects deterministic noise");
    check(noiseScenario.sensorNoiseAt(10) == noiseScenario.sensorNoiseAt(10),
          "noise profile is reproducible for repeatable debugging");
    SimulationScenario spikeScenario(Scenario::WorkloadSpike);
    check(spikeScenario.workloadAt(30) > spikeScenario.workloadAt(10),
          "workload-spike scenario applies a sudden high load");
}

void testThermalModelValidationAndDynamics() {
    using namespace virtualtherm;
    TemperatureSimulator simulator;
    const double initial = simulator.temperatureC();
    for (int i = 0; i < 20; ++i) simulator.advance(1.0, FanMode::Off);
    const double heated = simulator.temperatureC();
    check(heated > initial, "high workload heats the simulated plant");
    for (int i = 0; i < 30; ++i) simulator.advance(0.0, FanMode::High);
    check(simulator.temperatureC() < heated, "high fan cools the simulated plant");
    check(simulator.temperatureC() >= simulator.ambientTemperatureC(), "temperature does not fall below ambient");
    bool threw = false;
    try { simulator.advance(0.5, FanMode::Low, 0.0); }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw, "thermal model rejects non-positive time steps");
}

void testEndToEndScenarios() {
    using namespace virtualtherm;
    SimulationScenario heatScenario(Scenario::Overheat);
    TemperatureSimulator heatPlant;
    VirtualTemperatureSensor sensor;
    TemperatureFilter heatFilter;
    FanController heatController;
    bool sawHighFan = false;
    bool sawCritical = false;
    for (int t = 0; t < 150; ++t) {
        const auto reading = sensor.read(heatPlant.temperatureC(), t, heatScenario);
        const double filtered = heatFilter.update(reading);
        const auto output = heatController.update(reading, filtered);
        sawHighFan = sawHighFan || output.fanMode == FanMode::High;
        sawCritical = sawCritical || output.status == ControllerStatus::Critical;
        heatPlant.advance(heatScenario.workloadAt(t), output.fanMode);
    }
    check(sawHighFan, "overheat scenario eventually commands HIGH fan");
    check(sawCritical, "overheat scenario reaches the critical-temperature state");

    SimulationScenario faultScenario(Scenario::SensorFault);
    TemperatureSimulator faultPlant;
    TemperatureFilter faultFilter;
    FanController faultController;
    bool sawSensorFault = false;
    int invalidSamples = 0;
    bool allFaultSamplesCommandedHigh = true;
    for (int t = 0; t < 60; ++t) {
        const auto reading = sensor.read(faultPlant.temperatureC(), t, faultScenario);
        const double filtered = faultFilter.update(reading);
        const auto output = faultController.update(reading, filtered);
        if (!reading.valid) {
            ++invalidSamples;
            allFaultSamplesCommandedHigh = allFaultSamplesCommandedHigh && output.fanMode == FanMode::High;
        }
        sawSensorFault = sawSensorFault || output.status == ControllerStatus::SensorFault;
        faultPlant.advance(faultScenario.workloadAt(t), output.fanMode);
    }
    check(sawSensorFault, "sensor-fault scenario raises SENSOR_FAULT status");
    check(invalidSamples == 10, "sensor-fault scenario injects exactly ten invalid samples");
    check(allFaultSamplesCommandedHigh, "sensor-fault scenario maintains HIGH fan during every invalid sample");
}

}  // namespace

int main() {
    std::cout << "VirtualTherm automated tests\n=============================\n";
    testFanThresholdsAndHysteresis();
    testWarningAndCriticalLatch();
    testSensorFaultRecoveryDebounce();
    testFilter();
    testScenarioProfilesAndParsing();
    testThermalModelValidationAndDynamics();
    testEndToEndScenarios();
    if (failures != 0) {
        std::cerr << "\n" << failures << " test(s) failed.\n";
        return 1;
    }
    std::cout << "\nAll tests passed.\n";
    return 0;
}
