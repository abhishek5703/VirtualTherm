#include "virtualtherm/scenario.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace virtualtherm {

const char* toString(FanMode mode) noexcept {
    switch (mode) {
        case FanMode::Off: return "OFF";
        case FanMode::Low: return "LOW";
        case FanMode::Medium: return "MEDIUM";
        case FanMode::High: return "HIGH";
    }
    return "UNKNOWN";
}

const char* toString(ControllerStatus status) noexcept {
    switch (status) {
        case ControllerStatus::Normal: return "NORMAL";
        case ControllerStatus::Warning: return "WARNING";
        case ControllerStatus::Critical: return "CRITICAL";
        case ControllerStatus::SensorFault: return "SENSOR_FAULT";
    }
    return "UNKNOWN";
}

const char* toString(Scenario scenario) noexcept {
    switch (scenario) {
        case Scenario::Normal: return "normal";
        case Scenario::Overheat: return "overheat";
        case Scenario::SensorFault: return "sensor-fault";
        case Scenario::WorkloadSpike: return "workload-spike";
        case Scenario::NoisySensor: return "noisy-sensor";
    }
    return "unknown";
}

int fanPercent(FanMode mode) noexcept {
    switch (mode) {
        case FanMode::Off: return 0;
        case FanMode::Low: return 35;
        case FanMode::Medium: return 65;
        case FanMode::High: return 100;
    }
    return 100;
}

int estimatedFanRpm(FanMode mode) noexcept {
    switch (mode) {
        case FanMode::Off: return 0;
        case FanMode::Low: return 800;
        case FanMode::Medium: return 1400;
        case FanMode::High: return 2000;
    }
    return 2000;
}

SimulationScenario::SimulationScenario(Scenario scenario) : scenario_(scenario) {}
Scenario SimulationScenario::type() const noexcept { return scenario_; }
std::string SimulationScenario::name() const { return toString(scenario_); }

double SimulationScenario::workloadAt(int timeSeconds) const noexcept {
    const int t = std::max(0, timeSeconds);
    switch (scenario_) {
        case Scenario::Normal:
        case Scenario::NoisySensor:
            if (t < 10) return 0.25;
            if (t < 30) return 0.55;
            if (t < 70) return 0.70;
            if (t < 95) return 0.30;
            return 0.20;
        case Scenario::Overheat:
            if (t < 10) return 0.30;
            if (t < 110) return 1.00;
            return 0.20;
        case Scenario::SensorFault:
            if (t < 10) return 0.30;
            if (t < 35) return 0.65;
            if (t < 60) return 0.45;
            return 0.25;
        case Scenario::WorkloadSpike:
            if (t < 20) return 0.18;
            if (t < 55) return 1.00;
            if (t < 85) return 0.18;
            return 0.35;
    }
    return 0.0;
}

bool SimulationScenario::sensorValidAt(int timeSeconds) const noexcept {
    if (scenario_ == Scenario::SensorFault) {
        return !(timeSeconds >= 35 && timeSeconds < 45);
    }
    return true;
}

double SimulationScenario::sensorNoiseAt(int timeSeconds) const noexcept {
    if (scenario_ != Scenario::NoisySensor) return 0.0;
    const double t = static_cast<double>(std::max(0, timeSeconds));
    // Deterministic bounded noise so every run can be reproduced exactly.
    return 1.0 * std::sin(0.73 * t) + 0.35 * std::sin(1.91 * t);
}

std::optional<Scenario> SimulationScenario::parse(const std::string& name) noexcept {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    if (normalized == "normal") return Scenario::Normal;
    if (normalized == "overheat") return Scenario::Overheat;
    if (normalized == "sensor-fault" || normalized == "sensor_fault") return Scenario::SensorFault;
    if (normalized == "workload-spike" || normalized == "workload_spike") return Scenario::WorkloadSpike;
    if (normalized == "noisy-sensor" || normalized == "noisy_sensor") return Scenario::NoisySensor;
    return std::nullopt;
}

}  // namespace virtualtherm
