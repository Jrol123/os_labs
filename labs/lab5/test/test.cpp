#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include "http_server.h"
#include "network_publisher.h"
#include "network_collector.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>

// Обработчик данных для коллектора
void handleCollectedData(const std::string &data, TemperatureMonitor &monitor)
{
    // Парсим данные из формата: temperature=25.5&timestamp=1234567890&base=20.0&amplitude=5.0
    double temperature = 0.0;

    size_t temp_pos = data.find("temperature=");
    if (temp_pos != std::string::npos)
    {
        size_t end_pos = data.find("&", temp_pos);
        std::string temp_str = data.substr(temp_pos + 12, end_pos - (temp_pos + 12));
        temperature = std::stod(temp_str);

        // Логируем температуру
        monitor.logTemperature(temperature);
        std::cout << "Collected temperature: " << temperature << "°C" << std::endl;
    }
}

void testWithNetworkArchitecture(std::chrono::seconds test_duration,
                                 std::chrono::seconds measurement_interval)
{
    std::cout << "\n=== Testing with Network Architecture ===" << std::endl;

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
    server.setRefreshInterval(10);
    server.setMaxMeasurements(10);

    if (!server.start(8080))
    {
        std::cerr << "Failed to start HTTP server" << std::endl;
        return;
    }

    std::cout << "HTTP server started on port 8080" << std::endl;
    std::cout << "Open http://localhost:8080 in your browser" << std::endl;

    // Создаем эмулятор температуры
    TemperatureEmulator emulator(25.0, 1.0, 0.05);

    // Создаем и запускаем издателя
    NetworkPublisher publisher(5555);
    publisher.setDataCallback([&emulator]()
                              { return emulator.getTemperatureAsString(); });

    if (!publisher.start())
    {
        std::cerr << "Failed to start network publisher" << std::endl;
        return;
    }
    std::cout << "Network publisher started on port 5555" << std::endl;

    // Создаем и запускаем коллектор
    NetworkCollector collector("127.0.0.1", 5555);
    collector.setDataHandler([&](const std::string &data)
                             { handleCollectedData(data, TemperatureMonitor::getInstance()); });

    if (!collector.start())
    {
        std::cerr << "Failed to start network collector" << std::endl;
        return;
    }
    std::cout << "Network collector started" << std::endl;

    // Основной цикл публикации
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < test_duration)
    {
        // Публикуем данные
        std::string data = emulator.getTemperatureAsString();
        publisher.publish(data);

        std::this_thread::sleep_for(measurement_interval);
    }

    // Остановка
    collector.stop();
    publisher.stop();
    server.stop();
    TemperatureMonitor::getInstance().shutdown();

    std::cout << "Network architecture test completed" << std::endl;
}

int main()
{
    common::createDirectory("data");
    try
    {
        testWithNetworkArchitecture(std::chrono::seconds(300), std::chrono::seconds(1));
        std::cout << "\nAll tests completed successfully!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}