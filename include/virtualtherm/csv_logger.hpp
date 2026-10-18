#pragma once

#include "virtualtherm/types.hpp"

#include <fstream>
#include <string>

namespace virtualtherm {

class CsvLogger {
public:
    explicit CsvLogger(const std::string& path);
    ~CsvLogger();

    CsvLogger(const CsvLogger&) = delete;
    CsvLogger& operator=(const CsvLogger&) = delete;

    bool isOpen() const noexcept;
    const std::string& path() const noexcept;
    bool write(const SimulationSample& sample);

private:
    std::string path_;
    std::ofstream file_;
};

}  // namespace virtualtherm
