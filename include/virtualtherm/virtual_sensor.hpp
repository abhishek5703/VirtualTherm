#pragma once

#include "virtualtherm/types.hpp"

namespace virtualtherm {

class SimulationScenario;

// Turns a simulated plant temperature into a sensor reading. Fault windows
// and repeatable noise are injected by the selected scenario.
class VirtualTemperatureSensor {
public:
    SensorReading read(double plantTemperatureC,
                       int timeSeconds,
                       const SimulationScenario& scenario) const;
};

}  // namespace virtualtherm
