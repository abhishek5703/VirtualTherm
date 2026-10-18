#pragma once

#include "virtualtherm/types.hpp"

namespace virtualtherm {

// Stateful controller with hysteresis, a latched critical-temperature state,
// and three-good-sample recovery after sensor failure.
class FanController {
public:
    ControllerOutput update(const SensorReading& rawReading, double filteredTemperatureC);

    FanMode fanMode() const noexcept;
    ControllerStatus status() const noexcept;
    int consecutiveGoodSamples() const noexcept;

private:
    FanMode fanMode_{FanMode::Off};
    ControllerStatus status_{ControllerStatus::Normal};
    int consecutiveGoodSamples_{0};
    bool criticalLatched_{false};

    static bool readingIsUsable(const SensorReading& reading) noexcept;
    static FanMode nextFanMode(FanMode current, double temperatureC) noexcept;
    static ControllerStatus statusForTemperature(double temperatureC) noexcept;
};

}  // namespace virtualtherm
