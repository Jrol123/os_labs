#include "serial_port_wrapper.h"
#include "my_serial.hpp"

class SerialPortWrapper::Impl
{
public:
    std::unique_ptr<cplib::SerialPort> port;
};

SerialPortWrapper::SerialPortWrapper() : pimpl_(std::make_unique<Impl>()) {}

SerialPortWrapper::~SerialPortWrapper()
{
    close();
}

bool SerialPortWrapper::open(const std::string &port_name, int baud_rate)
{
    pimpl_->port = std::make_unique<cplib::SerialPort>();

    cplib::SerialPort::Parameters params;
    switch (baud_rate)
    {
    case 4800:
        params.baud_rate = cplib::SerialPort::BAUDRATE_4800;
        break;
    case 9600:
        params.baud_rate = cplib::SerialPort::BAUDRATE_9600;
        break;
    case 19200:
        params.baud_rate = cplib::SerialPort::BAUDRATE_19200;
        break;
    case 38400:
        params.baud_rate = cplib::SerialPort::BAUDRATE_38400;
        break;
    case 57600:
        params.baud_rate = cplib::SerialPort::BAUDRATE_57600;
        break;
    case 115200:
        params.baud_rate = cplib::SerialPort::BAUDRATE_115200;
        break;
    default:
        params.baud_rate = cplib::SerialPort::BAUDRATE_115200;
    }

    int result = pimpl_->port->Open(port_name, params);
    return result == cplib::SerialPort::RE_OK;
}

void SerialPortWrapper::close()
{
    if (pimpl_->port && pimpl_->port->IsOpen())
    {
        pimpl_->port->Close();
    }
    pimpl_->port.reset();
}

bool SerialPortWrapper::isOpen() const
{
    return pimpl_->port && pimpl_->port->IsOpen();
}

bool SerialPortWrapper::write(const std::string &data)
{
    if (!isOpen())
        return false;
    int result = pimpl_->port->Write(data);
    return result == cplib::SerialPort::RE_OK;
}

bool SerialPortWrapper::read(std::string &data, double timeout)
{
    if (!isOpen())
        return false;

    // Сохраняем текущий таймаут
    double old_timeout = pimpl_->port->GetTimeout();
    pimpl_->port->SetTimeout(timeout);

    int result = pimpl_->port->Read(data, timeout);

    // Восстанавливаем таймаут
    pimpl_->port->SetTimeout(old_timeout);

    return result == cplib::SerialPort::RE_OK && !data.empty();
}

void SerialPortWrapper::flush()
{
    if (isOpen())
    {
        pimpl_->port->Flush();
    }
}