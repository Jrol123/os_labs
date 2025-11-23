#ifndef TEMPERATURE_EMULATION_H
#define TEMPERATURE_EMULATION_H

#include <chrono>
#include <functional>
#include <random>
#include <memory>

class TemperatureEmulator
{
public:
    // Конструктор с настройками эмуляции
    TemperatureEmulator(double base_temp = 20.0,
                        double amplitude = 5.0,
                        double noise_level = 0.5);

    // Получить текущую температуру
    double getCurrentTemperature();

    // Установить базовую температуру
    void setBaseTemperature(double temp);

    // Установить амплитуду колебаний
    void setAmplitude(double amplitude);

    // Установить уровень шума
    void setNoiseLevel(double noise_level);

    // Симуляция суточных колебаний (true) или случайных (false)
    void enableDailyCycle(bool enable);

    // Установить собственный генератор температуры
    void setCustomGenerator(std::function<double()> generator);

private:
    double base_temperature_;
    double amplitude_;
    double noise_level_;
    bool daily_cycle_enabled_;

    std::minstd_rand random_engine_;
    std::normal_distribution<double> noise_distribution_;

    std::function<double()> custom_generator_;

    // Вспомогательные методы
    double generateDailyCycle();
    double generateRandomTemperature();
    double addNoise(double temperature);
};

#endif