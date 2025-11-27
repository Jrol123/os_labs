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

/// @brief Тестовая функция с использованием COM-порта
void testWithComPort(const std::string &emulator_port = "COM10",
                     const std::string &monitor_port = "COM11")
{
    std::cout << "\n=== Testing with COM Port Emulation ===" << std::endl;
    std::cout << "Emulator port: " << emulator_port << std::endl;
    std::cout << "Monitor port: " << monitor_port << std::endl;

    // Создаем эмулятор температуры
    auto temperature_emulator = createTemperatureEmulator(25.0, 1.0, 0.05);

    // Создаем и настраиваем эмулятор COM-порта
    ComPortEmulator com_emulator(emulator_port);
    com_emulator.setTemperatureEmulator(temperature_emulator);

    std::cout << "Starting COM port emulator..." << std::endl;
    if (!com_emulator.start())
    {
        std::cerr << "Failed to start COM port emulator on " << emulator_port << std::endl;
        std::cerr << "Make sure com0com is installed and ports are available" << std::endl;
        return;
    }

    // Настраиваем монитор температуры
    TemperatureMonitor::Config config;
    config.log_directory = "logs_com_port";
    config.measurement_interval = std::chrono::seconds(2);
    config.console_output = true;

    std::cout << "Initializing temperature monitor..." << std::endl;
    if (!TemperatureMonitor::getInstance().initialize(config))
    {
        std::cerr << "Failed to initialize temperature monitor" << std::endl;
        com_emulator.stop();
        return;
    }

    // Настраиваем COM-порт в мониторе
    std::cout << "Setting up COM port for monitor..." << std::endl;
    if (!TemperatureMonitor::getInstance().setupComPort(monitor_port))
    {
        std::cerr << "Failed to setup COM port " << monitor_port << " in monitor" << std::endl;
        std::cerr << "Available ports: COM1, COM2 (if using com0com)" << std::endl;
        com_emulator.stop();
        return;
    }

    // Запускаем чтение из COM-порта
    std::cout << "Starting COM port reading..." << std::endl;
    TemperatureMonitor::getInstance().startComPortReading();

    // Ждем некоторое время для демонстрации
    std::cout << "Running for 30 seconds..." << std::endl;
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

    std::cout << "COM port test completed successfully!" << std::endl;
}

/// @brief Тестовая функция (оригинальная)
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
    while (std::chrono::steady_clock::now() - start_time < time_test)
    {
        double temp = emulator.getCurrentTemperature();
        TemperatureMonitor::getInstance().logTemperature(temp);
        std::this_thread::sleep_for(config.measurement_interval);
    }

    std::cout << "Ultra-fast time test completed" << std::endl;
    common::resetToRealTime();
}

void printUsage()
{
    std::cout << "Usage:" << std::endl;
    std::cout << "  temperature_monitor [emulator_port] [monitor_port]" << std::endl;
    std::cout << "  Default ports: COM1 COM2" << std::endl;
    std::cout << std::endl;
    std::cout << "Make sure com0com is installed and virtual ports are created:" << std::endl;
    std::cout << "  setupc.exe install PortName=COM1 PortName=COM2" << std::endl;
}

int main(int argc, char *argv[])
{
    std::string emulator_port = "COM10";
    std::string monitor_port = "COM11";

    // Позволяем задать порты через аргументы командной строки
    if (argc >= 3)
    {
        emulator_port = argv[1];
        monitor_port = argv[2];
    }
    else if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"))
    {
        printUsage();
        return 0;
    }
    else if (argc != 1)
    {
        std::cerr << "Invalid arguments!" << std::endl;
        printUsage();
        return 1;
    }

    std::cout << "Temperature Monitor with COM Port Emulation" << std::endl;
    std::cout << "===========================================" << std::endl;

    try
    {
        // Тест с COM-портами
        testWithComPort(emulator_port, monitor_port);

        // Оригинальный тест
        std::cout << std::endl;
        testUltraFastTime(std::chrono::seconds(30), std::chrono::seconds(1),
                          std::chrono::seconds(5), std::chrono::seconds(10),
                          std::chrono::seconds(30));

        TemperatureMonitor::getInstance().shutdown();
        std::cout << "\nAll tests completed successfully!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        common::resetToRealTime();
        return 1;
    }

    return 0;
}