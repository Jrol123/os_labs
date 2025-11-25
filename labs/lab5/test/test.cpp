#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include "http_server.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <chrono>

void testWithHTTPServer(std::chrono::seconds test_duration,
                        std::chrono::seconds measurement_interval)
{
    std::cout << "\n=== Testing with HTTP Server ===" << std::endl;

    // Инициализация монитора
    TemperatureMonitor::Config config;
    config.console_output = true;
    config.measurement_interval = measurement_interval;

    if (!TemperatureMonitor::getInstance().initialize(config))
    {
        std::cerr << "Failed to initialize temperature monitor" << std::endl;
        return;
    }

    // Запуск HTTP сервера
    HTTPServer server(TemperatureMonitor::getInstance());

    server.setRefreshInterval(10); //! Не ставьте слишком маленькое значение, иначе браузер не будет просто успевать подгружать html-ку!
    server.setMaxMeasurements(10); // Показывать последние 20 измерений

    if (!server.start(8080))
    {
        std::cerr << "Failed to start HTTP server" << std::endl;
        return;
    }

    std::cout << "HTTP server started on port 8080" << std::endl;
    std::cout << "Open http://localhost:8080 in your browser" << std::endl;

    // Эмулятор температуры
    TemperatureEmulator emulator(25.0, 1.0, 0.05);

    // Основной цикл измерений
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < test_duration)
    {
        double temp = emulator.getCurrentTemperature();
        TemperatureMonitor::getInstance().logTemperature(temp);
        std::this_thread::sleep_for(measurement_interval);
    }

    // Остановка
    server.stop();
    TemperatureMonitor::getInstance().shutdown();

    std::cout << "HTTP server test completed" << std::endl;
}

int main()
{
    try
    {
        testWithHTTPServer(std::chrono::seconds(30), std::chrono::seconds(1));
        std::cout << "\nAll tests completed successfully!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}