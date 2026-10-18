#pragma once

#include "virtualtherm/types.hpp"

#include <optional>
#include <string>

namespace virtualtherm {

class SimulationScenario {
public:
    explicit SimulationScenario(Scenario scenario);

    Scenario type() const noexcept;
    std::string name() const;
    double workloadAt(int timeSeconds) const noexcept;
    bool sensorValidAt(int timeSeconds) const noexcept;
    double sensorNoiseAt(int timeSeconds) const noexcept;

    static std::optional<Scenario> parse(const std::string& name) noexcept;

private:
    Scenario scenario_;
};

}  // namespace virtualtherm
