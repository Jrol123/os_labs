#include "temperature_emulation.h"
#include <cmath>
#include <chrono>
#include <iostream>
#include <sstream>

TemperatureEmulator::TemperatureEmulator(double base_temp,
                                         double amplitude,
                                         double noise_level)
    : base_temperature_(base_temp),
      amplitude_(amplitude),
      noise_level_(noise_level),
      daily_cycle_enabled_(true),
      random_engine_(std::random_device{}()),
      noise_distribution_(0.0, noise_level)
{
}

bool TemperatureEmulator::initializeCOMPort(const std::string &port_name)
{
    try
    {
        serial_port_ = std::make_unique<cplib::SerialPort>();
        cplib::SerialPort::Parameters params(cplib::SerialPort::BAUDRATE_115200);
        params.timeout = 1.0;

        std::cout << "Trying to open COM port: " << port_name << std::endl;
        int result = serial_port_->Open(port_name, params);
        if (result == cplib::SerialPort::RE_OK)
        {
            com_initialized_ = true;
            std::cout << "COM port " << port_name << " initialized successfully" << std::endl;
            return true;
        }
        else
        {
            std::cerr << "Failed to initialize COM port " << port_name << ", error: " << result << std::endl;
            return false;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception initializing COM port: " << e.what() << std::endl;
        return false;
    }
}

void TemperatureEmulator::sendTemperatureToPort(double temperature)
{
    if (!com_initialized_ || !serial_port_)
    {
        std::cerr << "COM port not initialized" << std::endl;
        return;
    }

    try
    {
        std::stringstream data;
        data << "TEMP:" << temperature << "\n";

        std::string data_str = data.str();

        std::cout << "Sending temperature to COM port: " << data_str;

        int result = serial_port_->Write(data_str);
        if (result != cplib::SerialPort::RE_OK)
        {
            std::cerr << "Failed to send temperature to COM port, error: " << result << std::endl;
        }
        else
        {
            std::cout << "Temperature sent successfully: " << temperature << "°C" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception sending temperature to COM port: " << e.what() << std::endl;
    }
}

void TemperatureEmulator::closeCOMPort()
{
    if (serial_port_ && serial_port_->IsOpen())
    {
        serial_port_->Close();
    }
    com_initialized_ = false;
}

double TemperatureEmulator::getCurrentTemperature()
{
    if (custom_generator_)
    {
        return custom_generator_();
    }

    // Дефолтный генератор. Вопрос только в том, стоило ли попытаться запихнуть его в `.h`...
    double temperature;
    if (daily_cycle_enabled_)
    {
        temperature = generateDailyCycle();
    }
    else
    {
        temperature = generateRandomTemperature();
    }

    return addNoise(temperature);
}

double TemperatureEmulator::generateDailyCycle()
{
    // Получаем текущее время
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = *std::localtime(&time_t);

    // Преобразуем время в долю дня (0-1)
    double day_fraction = (local_time.tm_hour * 3600 +
                           local_time.tm_min * 60 +
                           local_time.tm_sec) /
                          86400.0;

    // Синусоидальная модель: минимум ночью, максимум днем
    // Сдвиг, чтобы пик был в 14:00
    double phase_shift = 14.0 / 24.0;
    double cycle = std::sin(2 * M_PI * (day_fraction - phase_shift));

    return base_temperature_ + amplitude_ * cycle;
}

double TemperatureEmulator::generateRandomTemperature()
{
    // Случайная температура в пределах base +- amplitude
    std::uniform_real_distribution<double> temp_dist(
        base_temperature_ - amplitude_,
        base_temperature_ + amplitude_);
    return temp_dist(random_engine_);
}

double TemperatureEmulator::addNoise(double temperature)
{
    if (noise_level_ > 0.0)
    {
        temperature += noise_distribution_(random_engine_);
    }
    return temperature;
}

void TemperatureEmulator::setBaseTemperature(double temp)
{
    base_temperature_ = temp;
}

void TemperatureEmulator::setAmplitude(double amplitude)
{
    amplitude_ = amplitude;
}

void TemperatureEmulator::setNoiseLevel(double noise_level)
{
    noise_level_ = noise_level;
    noise_distribution_ = std::normal_distribution<double>(0.0, noise_level_);
}

void TemperatureEmulator::enableDailyCycle(bool enable)
{
    daily_cycle_enabled_ = enable;
}

void TemperatureEmulator::setCustomGenerator(std::function<double()> generator)
{
    custom_generator_ = generator;
}