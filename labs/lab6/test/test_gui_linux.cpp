#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include "http_server.h"
#include "common.h"
#include "gui_linux.h"
#include <iostream>
#include <thread>
#include <chrono>

void runTemperatureSystem(std::chrono::seconds test_duration,
                          std::chrono::seconds measurement_interval)
{
    std::cout << "Starting Temperature Monitoring System..." << std::endl;

    TemperatureMonitor::Config config;
    config.console_output = false;
    config.measurement_interval = measurement_interval;

    if (!TemperatureMonitor::getInstance().initialize(config))
    {
        std::cerr << "Failed to initialize temperature monitor" << std::endl;
        return;
    }

    // Start HTTP server in separate thread
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

        while (server.isRunning())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } });

    // Temperature emulator
    TemperatureEmulator emulator(25.0, 1.0, 0.05);

    // Main measurement loop
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < test_duration)
    {
        double temp = emulator.getCurrentTemperature();
        TemperatureMonitor::getInstance().logTemperature(temp);
        std::this_thread::sleep_for(measurement_interval);
    }

    http_server_thread.detach();
    TemperatureMonitor::getInstance().shutdown();
    std::cout << "Temperature system completed" << std::endl;
}

int main(int argc, char *argv[])
{
    common::createDirectory("data");
    try
    {
        // Start temperature system in separate thread
        std::thread temp_thread([]()
                                { runTemperatureSystem(std::chrono::seconds(3600), std::chrono::seconds(1)); });
        temp_thread.detach();

        // Initialize and run Linux GUI
        TemperatureGUILinux &gui = TemperatureGUILinux::getInstance();
        gui.setMonitor(&TemperatureMonitor::getInstance());

        if (!gui.initialize(argc, argv))
        {
            std::cerr << "Failed to initialize GUI" << std::endl;
            return 1;
        }

        return gui.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}