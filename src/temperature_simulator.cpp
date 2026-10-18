#include "virtualtherm/temperature_simulator.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace virtualtherm {

TemperatureSimulator::TemperatureSimulator(double ambientTemperatureC)
    : ambientTemperatureC_(ambientTemperatureC), temperatureC_(ambientTemperatureC) {
    if (!std::isfinite(ambientTemperatureC) || ambientTemperatureC < -40.0 || ambientTemperatureC > 100.0) {
        throw std::invalid_argument("Ambient temperature must be a finite value between -40 and 100 C.");
    }
}

double TemperatureSimulator::temperatureC() const noexcept { return temperatureC_; }
double TemperatureSimulator::ambientTemperatureC() const noexcept { return ambientTemperatureC_; }

void TemperatureSimulator::advance(double workload, FanMode mode, double dtSeconds) {
    if (!std::isfinite(workload) || !std::isfinite(dtSeconds) || dtSeconds <= 0.0) {
        throw std::invalid_argument("Workload and time step must be finite; time step must be positive.");
    }
    const double boundedWorkload = std::clamp(workload, 0.0, 1.0);
    const double fanFraction = static_cast<double>(fanPercent(mode)) / 100.0;
    const double temperatureAboveAmbient = temperatureC_ - ambientTemperatureC_;
    const double heating = heatingRateCPerSecond_ * boundedWorkload;
    const double cooling = (naturalCoolingCoefficient_ + fanCoolingCoefficient_ * fanFraction)
                           * temperatureAboveAmbient;
    temperatureC_ += (heating - cooling) * dtSeconds;
    temperatureC_ = std::clamp(temperatureC_, ambientTemperatureC_, 150.0);
}

}  // namespace virtualtherm
