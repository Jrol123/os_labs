#include "temperature_emulation.h"
#include <cmath>
#include <chrono>
#ifdef _WIN32
#include <corecrt_math_defines.h>
#endif

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