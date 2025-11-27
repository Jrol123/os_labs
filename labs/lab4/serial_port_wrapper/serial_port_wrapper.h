#ifndef SERIAL_PORT_WRAPPER_H
#define SERIAL_PORT_WRAPPER_H

#include <string>
#include <memory>

// Обертка над SerialPort для избежания проблем с включением заголовков
class SerialPortWrapper
{
public:
    SerialPortWrapper();
    ~SerialPortWrapper();

    bool open(const std::string &port_name, int baud_rate = 115200);
    void close();
    bool isOpen() const;
    bool write(const std::string &data);
    bool read(std::string &data, double timeout = 1.0);
    void flush();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

#endif