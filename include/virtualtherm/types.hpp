#pragma once

#include <string>

namespace virtualtherm {

enum class FanMode {
    Off,
    Low,
    Medium,
    High
};

enum class ControllerStatus {
    Normal,
    Warning,
    Critical,
    SensorFault
};

enum class Scenario {
    Normal,
    Overheat,
    SensorFault,
    WorkloadSpike,
    NoisySensor
};

struct SensorReading {
    bool valid{true};
    double temperatureC{25.0}; // Raw temperature reported by the virtual sensor.
    std::string faultCode{};
};

struct ControllerOutput {
    FanMode fanMode{FanMode::Off};
    ControllerStatus status{ControllerStatus::Normal};
    std::string event{};
};

struct SimulationSample {
    int timeSeconds{0};
    std::string scenario{};
    double workload{0.0};
    double plantTemperatureC{25.0}; // Visible only because this is a simulation.
    bool sensorValid{true};
    double rawSensorTemperatureC{25.0};
    double filteredTemperatureC{25.0};
    FanMode fanMode{FanMode::Off};
    int fanPercent{0};
    int fanRpmEstimate{0}; // Modelled RPM, not a hardware measurement.
    ControllerStatus status{ControllerStatus::Normal};
    std::string event{};
};

const char* toString(FanMode mode) noexcept;
const char* toString(ControllerStatus status) noexcept;
const char* toString(Scenario scenario) noexcept;
int fanPercent(FanMode mode) noexcept;
int estimatedFanRpm(FanMode mode) noexcept;

}  // namespace virtualtherm
