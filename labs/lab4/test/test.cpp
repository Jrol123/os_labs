// test.cpp - улучшим тестовую функцию
#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <chrono>

void testUltraFastTime(std::chrono::seconds time_test,
                       std::chrono::seconds measurement_interval,
                       std::chrono::seconds hour_length,
                       std::chrono::seconds day_length,
                       std::chrono::seconds year_length,
                       const std::string &emulator_com_port = "COM1",
                       const std::string &monitor_com_port = "COM2")
{
    std::cout << "\n=== Testing Ultra-Fast Time (COM port communication) ===" << std::endl;
    std::cout << "Emulator port: " << emulator_com_port << std::endl;
    std::cout << "Monitor port: " << monitor_com_port << std::endl;

    common::setupCustomTime(
        hour_length,
        day_length,
        year_length);

    // Инициализация монитора
    TemperatureMonitor::Config monitor_config;
    monitor_config.log_directory = "logs_ultrafast";
    monitor_config.measurement_interval = measurement_interval;
    monitor_config.console_output = true;
    monitor_config.com_port = monitor_com_port;

    if (!TemperatureMonitor::getInstance().initialize(monitor_config))
    {
        std::cerr << "Failed to initialize temperature monitor" << std::endl;
        return;
    }

    // Запускаем чтение из COM-порта в мониторе
    if (!TemperatureMonitor::getInstance().startReadingFromCOMPort())
    {
        std::cerr << "Failed to start COM port reading in monitor" << std::endl;
        return;
    }

    // Даем время монитору начать чтение
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Инициализация эмулятора
    TemperatureEmulator emulator(25.0, 1.0, 0.05);

    if (!emulator.initializeCOMPort(emulator_com_port))
    {
        std::cerr << "Failed to initialize COM port in emulator" << std::endl;
        TemperatureMonitor::getInstance().stopReadingFromCOMPort();
        return;
    }

    std::cout << "Starting temperature transmission..." << std::endl;
    std::cout << "Test duration: " << time_test.count() << " seconds" << std::endl;
    std::cout << "Measurement interval: " << measurement_interval.count() << " seconds" << std::endl;

    auto start_time = std::chrono::steady_clock::now();
    int measurement_count = 0;

    while (std::chrono::steady_clock::now() - start_time < time_test)
    {
        double temp = emulator.getCurrentTemperature();
        emulator.sendTemperatureToPort(temp); // Отправляем через COM-порт
        measurement_count++;

        std::this_thread::sleep_for(measurement_interval);
    }

    std::cout << "Test completed. Sent " << measurement_count << " measurements." << std::endl;

    emulator.closeCOMPort();

    // Даем время на обработку оставшихся данных
    std::this_thread::sleep_for(std::chrono::seconds(1));

    common::resetToRealTime();
}

int main(int argc, char *argv[])
{
    std::string emulator_port = "COM5";
    std::string monitor_port = "COM6";

    // Позволяем задать COM порты через аргументы командной строки
    if (argc >= 3)
    {
        emulator_port = argv[1];
        monitor_port = argv[2];
        std::cout << "Using COM ports - Emulator: " << emulator_port << ", Monitor: " << monitor_port << std::endl;
    }
    else
    {
        std::cout << "Using default COM ports - Emulator: " << emulator_port << ", Monitor: " << monitor_port << std::endl;
        std::cout << "To specify ports: " << argv[0] << " COM1 COM2" << std::endl;
    }

    try
    {
        testUltraFastTime(std::chrono::seconds(60), // Уменьшим время теста для отладки
                          std::chrono::seconds(1),
                          std::chrono::seconds(5),
                          std::chrono::seconds(10),
                          std::chrono::seconds(30),
                          emulator_port,
                          monitor_port);

        TemperatureMonitor::getInstance().shutdown();
        std::cout << "\nTest completed successfully!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        common::resetToRealTime();
        return 1;
    }

    return 0;
}