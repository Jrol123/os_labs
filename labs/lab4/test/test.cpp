#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <chrono>

void testUltraFastTime()
{
    std::cout << "\n=== Testing Ultra-Fast Time (для демонстрации ротации) ===" << std::endl;

    common::setupCustomTime(
        std::chrono::seconds(1),  // 1 час = 1 секунда
        std::chrono::seconds(5), // 1 день = 5 секунд
        std::chrono::seconds(10)  // 1 год = 10 секунд
    );

    TemperatureMonitor::Config config;
    config.log_directory = "logs_ultrafast";
    config.measurement_interval = std::chrono::seconds(1);
    config.console_output = true;

    if (!TemperatureMonitor::getInstance().initialize(config))
    {
        std::cerr << "Failed to initialize temperature monitor" << std::endl;
        return;
    }

    TemperatureEmulator emulator(25.0, 1.0, 0.05);

    // Запускаем на 30 секунд реального времени
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(60))
    {
        double temp = emulator.getCurrentTemperature();
        TemperatureMonitor::getInstance().logTemperature(temp);
        std::this_thread::sleep_for(config.measurement_interval);
    }

    std::cout << "Ultra-fast time test completed" << std::endl;
    common::resetToRealTime();
}

int main()
{
    /*
    Чистка будет происходить после определённого числа тиков, а не в полночь
    */
    try
    {
        testUltraFastTime();

        TemperatureMonitor::getInstance().shutdown();
        std::cout << "\nAll time scale tests completed successfully!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        common::resetToRealTime();
        return 1;
    }

    return 0;
}