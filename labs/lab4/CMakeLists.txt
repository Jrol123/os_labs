cmake_minimum_required(VERSION 4.1)
project(TemperatureMonitor)

# add_library(temperature_monitor STATIC src/temperature_monitor.cpp include/temperature_monitor.h)
add_library(temperature_emulation STATIC temperature_emulation/temperature_emulation.cpp temperature_emulation/temperature_emulation.h)
# target_include_directories(temperature_monitor PUBLIC include)
target_include_directories(temperature_emulation PUBLIC temperature_emulation)

add_executable(LAB test/test.cpp)
# target_link_libraries(LAB_4 temperature_monitor)
target_link_libraries(LAB temperature_emulation)