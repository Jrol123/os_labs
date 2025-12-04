cmake_minimum_required(VERSION 4.1)
project(TemperatureMonitor)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")
endif()

find_package(PkgConfig REQUIRED)
# if(WIN32)
    find_file(SQLite3 NAMES sqlite3d.dll)
# else()
    # pkg_check_modules(sqlite3 REQUIRED sqlite3)
# endif()

add_library(time_manager STATIC time_manager/time_manager.cpp time_manager/time_manager.h)
target_include_directories(time_manager PUBLIC time_manager)

add_library(common STATIC common/common.cpp common/common.h)
target_include_directories(common PUBLIC common)

add_library(sqlite3 STATIC sqlite3/sqlite3.c sqlite3/sqlite3.h sqlite3/sqlite3ext.h)
target_include_directories(sqlite3 PUBLIC sqlite3)

add_library(database STATIC database/database.cpp database/database.h)
target_include_directories(database PUBLIC database)
target_link_libraries(database PRIVATE ${SQLite3_LIBRARIES})
target_include_directories(database PUBLIC ${SQLite3_INCLUDE_DIRS})

add_library(temperature_monitor STATIC temperature_monitor/temperature_monitor.cpp temperature_monitor/temperature_monitor.h)
target_include_directories(temperature_monitor PUBLIC temperature_monitor)

add_library(temperature_emulation STATIC temperature_emulation/temperature_emulation.cpp temperature_emulation/temperature_emulation.h)
target_include_directories(temperature_emulation PUBLIC temperature_emulation)

add_library(http_server STATIC http_server/http_server.cpp http_server/http_server.h)
target_include_directories(http_server PUBLIC http_server)

add_library(network_publisher STATIC network_publisher/network_publisher.cpp network_publisher/network_publisher.h)
target_include_directories(network_publisher PUBLIC network_publisher)
add_library(network_collector STATIC network_collector/network_collector.cpp network_collector/network_collector.h)
target_include_directories(network_collector PUBLIC network_collector)

target_link_libraries(network_publisher PUBLIC common)
target_link_libraries(network_collector PUBLIC common)


target_link_libraries(common PUBLIC time_manager)
target_link_libraries(database PUBLIC common)
target_link_libraries(database PUBLIC sqlite3)
target_link_libraries(time_manager PUBLIC common)
target_link_libraries(temperature_monitor PUBLIC common)
target_link_libraries(temperature_monitor PUBLIC database)
target_link_libraries(temperature_monitor PUBLIC time_manager)
target_link_libraries(temperature_emulation PUBLIC common)
target_link_libraries(http_server temperature_monitor)

if(WIN32)
    target_link_libraries(http_server ws2_32)
endif()

add_executable(LAB test/test.cpp)
target_link_libraries(LAB common)
target_link_libraries(LAB sqlite3)
target_link_libraries(LAB database)
target_link_libraries(LAB temperature_monitor)
target_link_libraries(LAB temperature_emulation)
target_link_libraries(LAB time_manager)
target_link_libraries(LAB http_server)
target_link_libraries(LAB network_publisher)
target_link_libraries(LAB network_collector)


if(WIN32)
    target_link_libraries(LAB ws2_32)
endif()

configure_file(
    http_server/template.html
    template.html
    COPYONLY
)