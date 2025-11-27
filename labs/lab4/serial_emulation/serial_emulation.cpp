#include "serial_emulation.h"
#include <iostream>
#include <chrono>
#include <sstream>

ComPortEmulator::ComPortEmulator(const std::string &port_name)
    : port_name_(port_name)
{
}

ComPortEmulator::~ComPortEmulator()
{
    stop();
}

bool ComPortEmulator::start()
{
    if (running_)
        return true;

    if (!serial_port_.open(port_name_))
    {
        std::cerr << "Failed to open COM port: " << port_name_ << std::endl;
        return false;
    }

    running_ = true;
    emulation_thread_ = std::thread(&ComPortEmulator::emulationThread, this);

    std::cout << "COM port emulator started on: " << port_name_ << std::endl;
    return true;
}

void ComPortEmulator::stop()
{
    if (!running_)
        return;

    running_ = false;

    if (emulation_thread_.joinable())
    {
        emulation_thread_.join();
    }

    serial_port_.close();

    std::cout << "COM port emulator stopped" << std::endl;
}

void ComPortEmulator::setTemperatureEmulator(std::shared_ptr<TemperatureEmulator> emulator)
{
    temperature_emulator_ = emulator;
}

void ComPortEmulator::emulationThread()
{
    while (running_)
    {
        if (temperature_emulator_)
        {
            double temperature = temperature_emulator_->getCurrentTemperature();

            std::stringstream data;
            data << "TEMP:" << temperature << "\n";

            if (!serial_port_.write(data.str()))
            {
                std::cerr << "Failed to write to COM port" << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}