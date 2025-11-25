cmake_minimum_required(VERSION 4.1)
project(TemperatureMonitor)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")
endif()

add_library(time_manager STATIC time_manager/time_manager.cpp time_manager/time_manager.h)
target_include_directories(time_manager PUBLIC time_manager)

add_library(common STATIC common/common.cpp common/common.h)
target_include_directories(common PUBLIC common)

add_library(temperature_monitor STATIC temperature_monitor/temperature_monitor.cpp temperature_monitor/temperature_monitor.h)
target_include_directories(temperature_monitor PUBLIC temperature_monitor)

add_library(temperature_emulation STATIC temperature_emulation/temperature_emulation.cpp temperature_emulation/temperature_emulation.h)
target_include_directories(temperature_emulation PUBLIC temperature_emulation)

add_library(http_server STATIC http_server/http_server.cpp http_server/http_server.h)
target_include_directories(http_server PUBLIC http_server)

target_link_libraries(common PUBLIC time_manager)
target_link_libraries(time_manager PUBLIC common)
target_link_libraries(temperature_monitor PUBLIC common)
target_link_libraries(temperature_monitor PUBLIC time_manager)
target_link_libraries(temperature_emulation PUBLIC common)
target_link_libraries(http_server temperature_monitor)

if(WIN32)
    target_link_libraries(http_server ws2_32)
endif()

add_executable(LAB test/test.cpp)
target_link_libraries(LAB common)
target_link_libraries(LAB temperature_monitor)
target_link_libraries(LAB temperature_emulation)
target_link_libraries(LAB time_manager)
target_link_libraries(LAB http_server)

if(WIN32)
    target_link_libraries(LAB ws2_32)
endif()

configure_file(
    http_server/template.html
    template.html
    COPYONLY
)