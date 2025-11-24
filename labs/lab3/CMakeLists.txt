cmake_minimum_required(VERSION 4.1)
project(SharedLogger)

include_directories(shared)

add_library(logger STATIC logger/logger.cpp logger/logger.hpp)
target_include_directories(logger PUBLIC logger)
add_library(shared_counter STATIC shared/shared_counter/shared_counter.cpp shared/shared_counter/shared_counter.hpp)
target_include_directories(shared_counter PUBLIC shared/shared_counter)

add_executable(LAB test/test.cpp)
target_link_libraries(LAB logger shared_counter)