cmake_minimum_required(VERSION 4.1)
project(BackgroundLauncher)

add_library(background_launcher STATIC src/background_launcher.cpp include/background_launcher.h)

target_include_directories(background_launcher PUBLIC include)

add_executable(LAB_2 test/test.cpp)
target_link_libraries(LAB_2 background_launcher)