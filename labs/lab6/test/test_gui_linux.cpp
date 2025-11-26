#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include "http_server.h"
#include "common.h"
#include "gui_linux.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

std::atomic<bool> shutdown_requested{false};

void signalHandler(int signal)
{
    std::cout << "Received shutdown signal..." << std::endl;
    shutdown_requested = true;
}

void runTemperatureSystem(std::chrono::seconds measurement_interval)
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

    // Temperature emulator
    TemperatureEmulator emulator(25.0, 1.0, 0.05);

    // Main measurement loop
    while (!shutdown_requested)
    {
        try
        {
            double temp = emulator.getCurrentTemperature();
            TemperatureMonitor::getInstance().logTemperature(temp);
            std::this_thread::sleep_for(measurement_interval);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in temperature system: " << e.what() << std::endl;
        }
    }

    TemperatureMonitor::getInstance().shutdown();
    std::cout << "Temperature system stopped" << std::endl;
}

void runHttpServer()
{
    try
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

        while (server.isRunning() && !shutdown_requested)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        server.stop();
        std::cout << "HTTP server stopped" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in HTTP server: " << e.what() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    // Setup signal handling
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    common::createDirectory("data");

    try
    {
        // Initialize monitor first
        TemperatureMonitor::Config config;
        config.console_output = false;
        config.measurement_interval = std::chrono::seconds(1);

        if (!TemperatureMonitor::getInstance().initialize(config))
        {
            std::cerr << "Failed to initialize temperature monitor" << std::endl;
            return 1;
        }

        // Start HTTP server in separate thread
        std::thread http_thread([]()
                                { runHttpServer(); });

        // Start temperature system in separate thread
        std::thread temp_thread([]()
                                { runTemperatureSystem(std::chrono::seconds(1)); });

        // Initialize and run Linux GUI
        TemperatureGUILinux &gui = TemperatureGUILinux::getInstance();
        gui.setMonitor(&TemperatureMonitor::getInstance());

        if (!gui.initialize(argc, argv))
        {
            std::cerr << "Failed to initialize GUI" << std::endl;
            shutdown_requested = true;
        }
        else
        {
            std::cout << "Starting GUI main loop..." << std::endl;
            gui.run();
        }

        // Cleanup
        shutdown_requested = true;

        if (http_thread.joinable())
        {
            http_thread.join();
        }

        if (temp_thread.joinable())
        {
            temp_thread.join();
        }

        std::cout << "Application shutdown complete" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}