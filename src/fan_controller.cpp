#include "virtualtherm/fan_controller.hpp"

#include <cmath>
#include <string>

namespace virtualtherm {
namespace {

void appendEvent(std::string& target, const std::string& event) {
    if (event.empty()) return;
    if (!target.empty()) target += ";";
    target += event;
}

}  // namespace

bool FanController::readingIsUsable(const SensorReading& reading) noexcept {
    return reading.valid && std::isfinite(reading.temperatureC)
           && reading.temperatureC >= -40.0 && reading.temperatureC <= 150.0;
}

FanMode FanController::nextFanMode(FanMode current, double temperatureC) noexcept {
    // Separate rising and falling thresholds provide hysteresis.
    switch (current) {
        case FanMode::Off:
            if (temperatureC >= 70.0) return FanMode::High;
            if (temperatureC >= 55.0) return FanMode::Medium;
            if (temperatureC >= 40.0) return FanMode::Low;
            return FanMode::Off;
        case FanMode::Low:
            if (temperatureC >= 70.0) return FanMode::High;
            if (temperatureC >= 55.0) return FanMode::Medium;
            if (temperatureC <= 37.0) return FanMode::Off;
            return FanMode::Low;
        case FanMode::Medium:
            if (temperatureC >= 70.0) return FanMode::High;
            if (temperatureC <= 37.0) return FanMode::Off;
            if (temperatureC <= 52.0) return FanMode::Low;
            return FanMode::Medium;
        case FanMode::High:
            if (temperatureC <= 37.0) return FanMode::Off;
            if (temperatureC <= 52.0) return FanMode::Low;
            if (temperatureC <= 67.0) return FanMode::Medium;
            return FanMode::High;
    }
    return FanMode::High;
}

ControllerStatus FanController::statusForTemperature(double temperatureC) noexcept {
    if (temperatureC >= 55.0) return ControllerStatus::Warning;
    return ControllerStatus::Normal;
}

ControllerOutput FanController::update(const SensorReading& rawReading, double filteredTemperatureC) {
    const FanMode oldFanMode = fanMode_;
    const ControllerStatus oldStatus = status_;
    std::string event;

    const bool filteredUsable = std::isfinite(filteredTemperatureC)
                                && filteredTemperatureC >= -40.0 && filteredTemperatureC <= 150.0;
    if (!readingIsUsable(rawReading) || !filteredUsable) {
        consecutiveGoodSamples_ = 0;
        status_ = ControllerStatus::SensorFault;
        fanMode_ = FanMode::High;
        if (oldStatus != ControllerStatus::SensorFault) {
            const std::string reason = rawReading.faultCode.empty() ? "INVALID_READING" : rawReading.faultCode;
            appendEvent(event, "SENSOR_FAULT:" + reason);
        }
    } else {
        // Require three consecutive plausible samples before leaving sensor-fault mode.
        if (oldStatus == ControllerStatus::SensorFault) {
            ++consecutiveGoodSamples_;
            if (consecutiveGoodSamples_ < 3) {
                status_ = ControllerStatus::SensorFault;
                fanMode_ = FanMode::High;
                if (consecutiveGoodSamples_ == 1) appendEvent(event, "SENSOR_RECOVERY_STARTED");
            } else {
                consecutiveGoodSamples_ = 0;
                status_ = ControllerStatus::Normal;
                appendEvent(event, "SENSOR_RECOVERED");
            }
        } else {
            consecutiveGoodSamples_ = 0;
        }

        if (status_ != ControllerStatus::SensorFault) {
            // A raw over-temperature reading must not wait for the filter to catch up.
            if (rawReading.temperatureC >= 78.0 || filteredTemperatureC >= 78.0) {
                criticalLatched_ = true;
            } else if (criticalLatched_ && filteredTemperatureC <= 72.0
                       && rawReading.temperatureC <= 72.0) {
                criticalLatched_ = false;
                appendEvent(event, "CRITICAL_TEMPERATURE_CLEARED");
            }

            if (criticalLatched_) {
                status_ = ControllerStatus::Critical;
                fanMode_ = FanMode::High;
            } else {
                status_ = statusForTemperature(filteredTemperatureC);
                fanMode_ = nextFanMode(fanMode_, filteredTemperatureC);
            }
        }
    }

    if (oldFanMode != fanMode_) {
        appendEvent(event, std::string("FAN_") + toString(oldFanMode) + "_TO_" + toString(fanMode_));
    }
    if (oldStatus != status_) {
        appendEvent(event, std::string("STATUS_") + toString(oldStatus) + "_TO_" + toString(status_));
    }
    return ControllerOutput{fanMode_, status_, event};
}

FanMode FanController::fanMode() const noexcept { return fanMode_; }
ControllerStatus FanController::status() const noexcept { return status_; }
int FanController::consecutiveGoodSamples() const noexcept { return consecutiveGoodSamples_; }

}  // namespace virtualtherm
