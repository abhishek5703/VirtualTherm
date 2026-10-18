#pragma once

#include "virtualtherm/types.hpp"

namespace virtualtherm {

// A simple first-order thermal model for software-in-the-loop experimentation.
// It is illustrative and not calibrated to a real machine.
class TemperatureSimulator {
public:
    explicit TemperatureSimulator(double ambientTemperatureC = 25.0);

    double temperatureC() const noexcept;
    double ambientTemperatureC() const noexcept;

    // Advances by dtSeconds using normalized workload (0..1) and fan command.
    void advance(double workload, FanMode fanMode, double dtSeconds = 1.0);

private:
    double ambientTemperatureC_;
    double temperatureC_;
    double heatingRateCPerSecond_{0.90};
    double naturalCoolingCoefficient_{0.006};
    double fanCoolingCoefficient_{0.006};
};

}  // namespace virtualtherm
