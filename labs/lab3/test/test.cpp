#include "logger.hpp"
#include "shared_counter.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    cplib::Logger logger("process.log");
    logger.logStartup();
    
    cplib::SharedCounter counter("global_counter");
    if (!counter.isValid()) {
        std::cerr << "Failed to create shared counter!" << std::endl;
        return -1;
    }
    
    std::cout << "Process started. PID logged to file." << std::endl;
    std::cout << "Initial counter value: " << counter.getValue() << std::endl;
    
    // Простая демонстрация
    counter.increment();
    std::cout << "After increment: " << counter.getValue() << std::endl;
    
    logger.logCounter(counter.getValue());
    
    return 0;
}