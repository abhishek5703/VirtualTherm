#include "virtualtherm/virtual_sensor.hpp"
#include "virtualtherm/scenario.hpp"

#include <cmath>
#include <limits>

namespace virtualtherm {

SensorReading VirtualTemperatureSensor::read(double plantTemperatureC,
                                              int timeSeconds,
                                              const SimulationScenario& scenario) const {
    if (!scenario.sensorValidAt(timeSeconds)) {
        return SensorReading{false, std::numeric_limits<double>::quiet_NaN(), "SIMULATED_SENSOR_DISCONNECT"};
    }
    if (!std::isfinite(plantTemperatureC)) {
        return SensorReading{false, std::numeric_limits<double>::quiet_NaN(), "NON_FINITE_SENSOR_VALUE"};
    }
    const double measured = plantTemperatureC + scenario.sensorNoiseAt(timeSeconds);
    if (!std::isfinite(measured)) {
        return SensorReading{false, std::numeric_limits<double>::quiet_NaN(), "NON_FINITE_SENSOR_VALUE"};
    }
    return SensorReading{true, measured, {}};
}

}  // namespace virtualtherm
