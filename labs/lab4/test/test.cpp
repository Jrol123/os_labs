#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include "serial_emulation.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

// Функция для создания эмулятора температуры
std::shared_ptr<TemperatureEmulator> createTemperatureEmulator(
    double base_temp = 20.0, double amplitude = 5.0, double noise_level = 0.5)
{
    return std::make_shared<TemperatureEmulator>(base_temp, amplitude, noise_level);
}

/// @brief Тестовая функция с использованием COM-порта (ТОЛЬКО COM-порт)
void testComPortOnly(const std::string &emulator_port = "COM1",
                     const std::string &monitor_port = "COM2")
{
    std::cout << "\n=== Testing COM Port Only ===" << std::endl;
    std::cout << "Emulator port: " << emulator_port << std::endl;
    std::cout << "Monitor port: " << monitor_port << std::endl;

    // Настраиваем монитор температуры
    TemperatureMonitor::Config config;
    config.log_directory = "logs_com_port_only";
    config.measurement_interval = std::chrono::seconds(2);
    config.console_output = true;

    std::cout << "Initializing temperature monitor..." << std::endl;
    if (!TemperatureMonitor::getInstance().initialize(config))
    {
        std::cerr << "Failed to initialize temperature monitor" << std::endl;
        return;
    }

    // Настраиваем COM-порт в мониторе
    std::cout << "Setting up COM port for monitor..." << std::endl;
    if (!TemperatureMonitor::getInstance().setupComPort(monitor_port))
    {
        std::cerr << "Failed to setup COM port " << monitor_port << " in monitor" << std::endl;
        std::cerr << "Make sure com0com is installed and virtual ports are created" << std::endl;
        return;
    }

    // Создаем эмулятор температуры для COM-порта
    auto temperature_emulator = createTemperatureEmulator(25.0, 1.0, 0.05);

    // Создаем и настраиваем эмулятор COM-порта
    ComPortEmulator com_emulator(emulator_port);
    com_emulator.setTemperatureEmulator(temperature_emulator);

    std::cout << "Starting COM port emulator..." << std::endl;
    if (!com_emulator.start())
    {
        std::cerr << "Failed to start COM port emulator on " << emulator_port << std::endl;
        return;
    }

    // Запускаем чтение из COM-порта
    std::cout << "Starting COM port reading..." << std::endl;
    TemperatureMonitor::getInstance().startComPortReading();

    // Ждем некоторое время для демонстрации
    std::cout << "Running for 30 seconds (ONLY COM port data)..." << std::endl;
    for (int i = 0; i < 30; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "." << std::flush;
        if (i % 10 == 9)
            std::cout << " " << (i + 1) << "s" << std::endl;
    }

    // Останавливаем всё
    std::cout << "\nStopping COM port reading..." << std::endl;
    TemperatureMonitor::getInstance().stopComPortReading();
    com_emulator.stop();

    std::cout << "COM port only test completed!" << std::endl;
}

// /// @brief Тестовая функция с прямым вызовом (БЕЗ COM-порта)
// void testDirectOnly(std::chrono::seconds time_test, std::chrono::seconds measurement_interval)
// {
//     std::cout << "\n=== Testing Direct Calls Only (NO COM port) ===" << std::endl;

//     TemperatureMonitor::Config config;
//     config.log_directory = "logs_direct_only";
//     config.measurement_interval = measurement_interval;
//     config.console_output = true;

//     if (!TemperatureMonitor::getInstance().initialize(config))
//     {
//         std::cerr << "Failed to initialize temperature monitor" << std::endl;
//         return;
//     }

//     TemperatureEmulator emulator(22.0, 2.0, 0.1);

//     auto start_time = std::chrono::steady_clock::now();
//     int count = 0;
//     while (std::chrono::steady_clock::now() - start_time < time_test)
//     {
//         double temp = emulator.getCurrentTemperature();
//         TemperatureMonitor::getInstance().logTemperature(temp);
//         std::this_thread::sleep_for(config.measurement_interval);
//         count++;
//     }

//     std::cout << "Direct calls test completed! Logged " << count << " measurements." << std::endl;
// }

/// @brief Тестовая функция для демонстрации ротации
void testUltraFastTime(std::chrono::seconds time_test, std::chrono::seconds measurement_interval,
                       std::chrono::seconds hour_length, std::chrono::seconds day_length,
                       std::chrono::seconds year_length)
{
    std::cout << "\n=== Testing Ultra-Fast Time (для демонстрации ротации) ===" << std::endl;

    common::setupCustomTime(hour_length, day_length, year_length);

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
    int count = 0;
    while (std::chrono::steady_clock::now() - start_time < time_test)
    {
        double temp = emulator.getCurrentTemperature();
        TemperatureMonitor::getInstance().logTemperature(temp);
        std::this_thread::sleep_for(config.measurement_interval);
        count++;
    }

    std::cout << "Ultra-fast time test completed! Logged " << count << " measurements." << std::endl;
    common::resetToRealTime();
}

void printUsage()
{
    std::cout << "Usage:" << std::endl;
    std::cout << "  temperature_monitor [test_mode] [emulator_port] [monitor_port]" << std::endl;
    std::cout << "  test_mode: com, direct, ultra, all" << std::endl;
    std::cout << "  Default: all COM1 COM2" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  temperature_monitor com COM1 COM2     # Test COM ports only" << std::endl;
    std::cout << "  temperature_monitor direct            # Test direct calls only" << std::endl;
    std::cout << "  temperature_monitor ultra             # Test ultra-fast time only" << std::endl;
    std::cout << "  temperature_monitor all COM10 COM11   # Test all modes" << std::endl;
}

int main(int argc, char *argv[])
{
    std::string test_mode = "all";
    std::string emulator_port = "COM1";
    std::string monitor_port = "COM2";

    // Парсим аргументы командной строки
    if (argc >= 2)
    {
        test_mode = argv[1];
    }
    if (argc >= 4)
    {
        emulator_port = argv[2];
        monitor_port = argv[3];
    }
    else if (argc == 2 && (test_mode == "--help" || test_mode == "-h"))
    {
        printUsage();
        return 0;
    }

    std::cout << "Temperature Monitor Test" << std::endl;
    std::cout << "========================" << std::endl;
    std::cout << "Test mode: " << test_mode << std::endl;
    if (test_mode == "com" || test_mode == "all")
    {
        std::cout << "Emulator port: " << emulator_port << std::endl;
        std::cout << "Monitor port: " << monitor_port << std::endl;
    }

    try
    {
        if (test_mode == "com" || test_mode == "all")
        {
            testComPortOnly(emulator_port, monitor_port);
        }

        if (test_mode == "ultra" || test_mode == "all")
        {
            testUltraFastTime(std::chrono::seconds(30), std::chrono::seconds(1),
                              std::chrono::seconds(5), std::chrono::seconds(10),
                              std::chrono::seconds(30));
        }

        TemperatureMonitor::getInstance().shutdown();
        std::cout << "\nAll selected tests completed successfully!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        common::resetToRealTime();
        return 1;
    }

    return 0;
}