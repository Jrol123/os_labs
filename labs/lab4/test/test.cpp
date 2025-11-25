#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <chrono>

/// @brief Тестовая функция
/// @param time_test Сколько реального времени будет идти тест
/// @param measurement_interval Раз в какой промежуток времени будут проводиться измерения
/// @param hour_length Длина часа в реальном времени
/// @param day_length Длина дня в реальном времени
/// @param year_length Длина года в реальном времени
void testUltraFastTime(std::chrono::seconds time_test, std::chrono::seconds measurement_interval, std::chrono::seconds hour_length, std::chrono::seconds day_length, std::chrono::seconds year_length)
{
    std::cout << "\n=== Testing Ultra-Fast Time (для демонстрации ротации) ===" << std::endl;

    common::setupCustomTime(
        hour_length,
        day_length,
        year_length);

    TemperatureMonitor::Config config;
    config.log_directory = "logs_ultrafast";
    config.measurement_interval = measurement_interval;
    config.console_output = true;

    if (!TemperatureMonitor::getInstance().initialize(config))
    {
        std::cerr << "Failed to initialize temperature monitor" << std::endl;
        return;
    }

    TemperatureEmulator emulator(25.0, 1.0, 0.05);

    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < time_test)
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
        testUltraFastTime(std::chrono::seconds(30), std::chrono::seconds(1), std::chrono::seconds(5), std::chrono::seconds(10), std::chrono::seconds(30));

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