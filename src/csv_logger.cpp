#include "virtualtherm/csv_logger.hpp"

#include <iomanip>

namespace virtualtherm {
namespace {

std::string csvEscape(const std::string& value) {
    if (value.find_first_of(",\"\r\n") == std::string::npos) return value;
    std::string escaped = "\"";
    for (const char character : value) {
        if (character == '"') escaped += "\"\"";
        else escaped += character;
    }
    escaped += '"';
    return escaped;
}

}  // namespace

CsvLogger::CsvLogger(const std::string& path) : path_(path), file_(path, std::ios::out | std::ios::trunc) {
    if (file_.is_open()) {
        file_ << "time_s,scenario,workload,plant_temp_c,raw_sensor_temp_c,filtered_temp_c,sensor_valid,"
                 "fan_mode,fan_percent,fan_rpm_est,controller_status,event\n";
        file_ << std::fixed << std::setprecision(2);
    }
}

CsvLogger::~CsvLogger() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

bool CsvLogger::isOpen() const noexcept { return file_.is_open() && file_.good(); }
const std::string& CsvLogger::path() const noexcept { return path_; }

bool CsvLogger::write(const SimulationSample& sample) {
    if (!isOpen()) return false;
    file_ << sample.timeSeconds << ','
          << csvEscape(sample.scenario) << ','
          << sample.workload << ','
          << sample.plantTemperatureC << ',';
    if (sample.sensorValid) file_ << sample.rawSensorTemperatureC;
    file_ << ',';
    if (sample.sensorValid) file_ << sample.filteredTemperatureC;
    file_ << ','
          << (sample.sensorValid ? "true" : "false") << ','
          << toString(sample.fanMode) << ','
          << sample.fanPercent << ','
          << sample.fanRpmEstimate << ','
          << toString(sample.status) << ','
          << csvEscape(sample.event) << '\n';
    return file_.good();
}

}  // namespace virtualtherm
