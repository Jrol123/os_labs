#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include <iostream>
#include <thread>
#include <chrono>

void testBasicLogging() {
    std::cout << "=== Testing Basic Logging ===" << std::endl;
    
    // Инициализация логгера
    TemperatureMonitor::Config config;
    config.log_directory = "test_logs";
    config.measurement_interval = std::chrono::seconds(2); // Для тестов используем 2 секунды
    config.console_output = true;
    
    if (!TemperatureMonitor::getInstance().initialize(config)) {
        std::cerr << "Failed to initialize logger" << std::endl;
        return;
    }
    
    // Создаем эмулятор
    TemperatureEmulator emulator(22.0, 3.0, 0.1);
    
    // Делаем 5 измерений
    for (int i = 0; i < 5; ++i) {
        double temp = emulator.getCurrentTemperature();
        TemperatureMonitor::getInstance().logTemperature(temp);
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "Basic logging test completed" << std::endl;
}

void testCustomInterval() {
    std::cout << "\n=== Testing Custom Interval ===" << std::endl;
    
    // Меняем интервал на 500ms
    TemperatureMonitor::getInstance().setMeasurementInterval(std::chrono::milliseconds(500));
    
    TemperatureEmulator emulator(25.0, 1.0, 0.05);
    
    // Быстрые измерения
    for (int i = 0; i < 3; ++i) {
        double temp = emulator.getCurrentTemperature();
        TemperatureMonitor::getInstance().logTemperature(temp);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << "Custom interval test completed" << std::endl;
}

int main() {
    try {
        testBasicLogging();
        testCustomInterval();
        
        // Автоматически закроется при shutdown
        TemperatureMonitor::getInstance().shutdown();
        std::cout << "\nAll tests completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}