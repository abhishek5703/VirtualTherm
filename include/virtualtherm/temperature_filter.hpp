#pragma once

#include "virtualtherm/types.hpp"

namespace virtualtherm {

// Exponential moving average used to smooth small sensor fluctuations.
// A sensor failure clears filter history so stale values are not reused.
class TemperatureFilter {
public:
    explicit TemperatureFilter(double alpha = 0.35);

    double update(const SensorReading& reading) noexcept;
    void reset() noexcept;
    bool initialized() const noexcept;
    double valueC() const noexcept;
    double alpha() const noexcept;

private:
    double alpha_;
    double valueC_{0.0};
    bool initialized_{false};
};

}  // namespace virtualtherm
