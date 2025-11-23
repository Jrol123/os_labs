cmake_minimum_required(VERSION 4.1)
project(TemperatureMonitor)

add_library(common STATIC common/common.cpp common/common.h)
target_include_directories(common PUBLIC common)


add_library(temperature_monitor STATIC temperature_monitor/temperature_monitor.cpp temperature_monitor/temperature_monitor.h)
target_include_directories(temperature_monitor PUBLIC temperature_monitor)

add_library(temperature_emulation STATIC temperature_emulation/temperature_emulation.cpp temperature_emulation/temperature_emulation.h)
target_include_directories(temperature_emulation PUBLIC temperature_emulation)

add_executable(LAB test/test.cpp)
target_link_libraries(LAB common)
target_link_libraries(LAB temperature_monitor)
target_link_libraries(LAB temperature_emulation)

target_include_directories(LAB PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/temperature_emulation
    ${CMAKE_CURRENT_SOURCE_DIR}/temperature_monitor
)