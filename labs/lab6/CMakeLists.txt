cmake_minimum_required(VERSION 4.1)
project(TemperatureMonitor)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCMAKE_BUILD_TYPE=Debug")
endif()

# find_package(PkgConfig REQUIRED)
find_program(SQLite3 NAMES sqlite3.exe)
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(X11_FOUND)
        include_directories(${X11_INCLUDE_DIR})
        # Find Xt and other required X11 libraries
        find_library(XT_LIB Xt)
        find_library(XAW_LIB Xaw)
        find_library(XMU_LIB Xmu)
        find_library(XEXT_LIB Xext)
        
        set(X11_LIBRARIES 
            ${X11_LIBRARIES}
            ${XT_LIB}
            ${XAW_LIB} 
            ${XMU_LIB}
            ${XEXT_LIB}
        )
        message(STATUS "X11 libraries found: ${X11_LIBRARIES}")
    else()
        message(WARNING "X11 not found - GUI will not be available")
    endif()
endif()
# if(WIN32)
    # find_file(SQLite3 NAMES sqlite3d.dll)
# else()
    # pkg_check_modules(sqlite3 REQUIRED sqlite3)
# endif()

add_library(time_manager STATIC time_manager/time_manager.cpp time_manager/time_manager.h)
target_include_directories(time_manager PUBLIC time_manager)

add_library(common STATIC common/common.cpp common/common.h)
target_include_directories(common PUBLIC common)

if(WIN32)
    add_library(gui STATIC gui/gui.cpp gui/gui.h)
else()
    add_library(gui STATIC gui/gui_linux.cpp gui/gui_linux.h)
endif()
target_include_directories(gui PUBLIC gui)

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

target_link_libraries(common PUBLIC time_manager)
target_link_libraries(database PUBLIC common)
target_link_libraries(database PUBLIC sqlite3)
target_link_libraries(time_manager PUBLIC common)
target_link_libraries(temperature_monitor PUBLIC common)
target_link_libraries(temperature_monitor PUBLIC database)
target_link_libraries(temperature_monitor PUBLIC time_manager)
target_link_libraries(temperature_emulation PUBLIC common)
target_link_libraries(http_server temperature_monitor)
target_link_libraries(gui http_server)

if(WIN32)
    target_link_libraries(http_server ws2_32)
endif()

# CONSOLE

add_executable(LAB test/test.cpp)
target_link_libraries(LAB common)
target_link_libraries(LAB sqlite3)
target_link_libraries(LAB database)
target_link_libraries(LAB temperature_monitor)
target_link_libraries(LAB temperature_emulation)
target_link_libraries(LAB time_manager)
target_link_libraries(LAB http_server)


if(WIN32)
    target_link_libraries(LAB ws2_32)
endif()

# GUI
if(WIN32)
    add_executable(LAB_GUI test/test_gui.cpp)
else()
    add_executable(LAB_GUI test/test_gui_linux.cpp)
endif()
target_link_libraries(LAB_GUI common)
target_link_libraries(LAB_GUI sqlite3)
target_link_libraries(LAB_GUI database)
target_link_libraries(LAB_GUI temperature_monitor)
target_link_libraries(LAB_GUI temperature_emulation)
target_link_libraries(LAB_GUI time_manager)
target_link_libraries(LAB_GUI http_server)
target_link_libraries(LAB_GUI gui)


if(WIN32)
    target_link_libraries(http_server ws2_32)
    target_link_libraries(LAB_GUI ws2_32)
    target_link_libraries(LAB_GUI comctl32)
else()
    target_link_libraries(LAB_GUI
            ${X11_LIBRARIES}
            sqlite3
            pthread
        )
endif()

configure_file(
    http_server/template.html
    template.html
    COPYONLY
)