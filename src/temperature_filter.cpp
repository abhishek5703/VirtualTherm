#include "virtualtherm/temperature_filter.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace virtualtherm {

TemperatureFilter::TemperatureFilter(double alpha) : alpha_(alpha) {
    if (!std::isfinite(alpha_) || alpha_ <= 0.0 || alpha_ > 1.0) {
        throw std::invalid_argument("Filter alpha must be greater than 0 and no more than 1.");
    }
}

double TemperatureFilter::update(const SensorReading& reading) noexcept {
    if (!reading.valid || !std::isfinite(reading.temperatureC)
        || reading.temperatureC < -40.0 || reading.temperatureC > 150.0) {
        reset();
        return std::numeric_limits<double>::quiet_NaN();
    }

    if (!initialized_) {
        valueC_ = reading.temperatureC;
        initialized_ = true;
    } else {
        valueC_ = alpha_ * reading.temperatureC + (1.0 - alpha_) * valueC_;
    }
    return valueC_;
}

void TemperatureFilter::reset() noexcept {
    valueC_ = 0.0;
    initialized_ = false;
}

bool TemperatureFilter::initialized() const noexcept { return initialized_; }
double TemperatureFilter::valueC() const noexcept { return initialized_ ? valueC_ : std::numeric_limits<double>::quiet_NaN(); }
double TemperatureFilter::alpha() const noexcept { return alpha_; }

}  // namespace virtualtherm
