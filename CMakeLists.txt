cmake_minimum_required(VERSION 3.16)
project(VirtualTherm VERSION 1.1.0 LANGUAGES CXX)

option(VIRTUALTHERM_BUILD_TESTS "Build the lightweight unit tests" ON)

add_library(virtualtherm_core
    src/temperature_simulator.cpp
    src/temperature_filter.cpp
    src/virtual_sensor.cpp
    src/fan_controller.cpp
    src/csv_logger.cpp
    src/scenario.cpp
)
target_include_directories(virtualtherm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(virtualtherm_core PUBLIC cxx_std_17)

function(virtualtherm_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

virtualtherm_enable_warnings(virtualtherm_core)

add_executable(virtualtherm src/main.cpp)
target_link_libraries(virtualtherm PRIVATE virtualtherm_core)
virtualtherm_enable_warnings(virtualtherm)

if(VIRTUALTHERM_BUILD_TESTS)
    enable_testing()
    add_executable(virtualtherm_tests tests/test_main.cpp)
    target_link_libraries(virtualtherm_tests PRIVATE virtualtherm_core)
    virtualtherm_enable_warnings(virtualtherm_tests)
    add_test(NAME virtualtherm_tests COMMAND virtualtherm_tests)
endif()
