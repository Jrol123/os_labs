#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include "http_server.h"
#include "common.h"
#include "gui.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>

void runTemperatureSystem(std::chrono::seconds test_duration,
                          std::chrono::seconds measurement_interval)
{
    std::cout << "Starting Temperature Monitoring System..." << std::endl;
    // Инициализация монитора
    TemperatureMonitor::Config config;
    config.console_output = false; // Отключаем вывод в консоль для GUI
    config.measurement_interval = measurement_interval;

    if (!TemperatureMonitor::getInstance().initialize(config))
    {
        std::cerr << "Failed to initialize temperature monitor" << std::endl;
        return;
    }

    // Запуск HTTP сервера в отдельном потоке
    std::thread http_server_thread([]()
                                   {
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

    // HTTP сервер будет работать до завершения приложения
    while (server.isRunning())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    } });

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

    // Остановка HTTP сервера
    http_server_thread.detach(); // Отсоединяем поток, так как он завершится с приложением

    TemperatureMonitor::getInstance().shutdown();
    std::cout << "Temperature system completed" << std::endl;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    common::createDirectory("data");
    try
    {
        // Запуск системы мониторинга температуры в отдельном потоке
        std::thread temp_thread([]()
                                { runTemperatureSystem(std::chrono::seconds(3600), std::chrono::seconds(1)); });
        temp_thread.detach();

        // Инициализация и запуск GUI
        TemperatureGUI &gui = TemperatureGUI::getInstance();
        gui.setMonitor(&TemperatureMonitor::getInstance());

        if (!gui.initialize(hInstance))
        {
            MessageBox(NULL, (LPCSTR) "Failed to initialize GUI", (LPCSTR) "Error", MB_ICONERROR);
            return 1;
        }

        return gui.run();
    }
    catch (const std::exception &e)
    {
        std::string error_msg = "Error: " + std::string(e.what());
        std::wstring werror_msg = TemperatureGUI::getInstance().stringToWString(error_msg);
        MessageBox(NULL, (LPCSTR)werror_msg.c_str(), (LPCSTR) "Error", MB_ICONERROR);
        return 1;
    }

    return 0;
}