#ifndef SERIAL_EMULATION_H
#define SERIAL_EMULATION_H

#include "temperature_emulation.h"
#include "serial_port_wrapper.h"
#include <thread>
#include <atomic>
#include <memory>

class ComPortEmulator
{
public:
    ComPortEmulator(const std::string &port_name = "COM1");
    ~ComPortEmulator();

    bool start();
    void stop();
    void setTemperatureEmulator(std::shared_ptr<TemperatureEmulator> emulator);
    bool isRunning() const { return running_; }

private:
    void emulationThread();

    std::string port_name_;
    std::shared_ptr<TemperatureEmulator> temperature_emulator_;
    SerialPortWrapper serial_port_;
    std::thread emulation_thread_;
    std::atomic<bool> running_{false};
};

#endif