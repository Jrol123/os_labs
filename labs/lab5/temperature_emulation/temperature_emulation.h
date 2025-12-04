#ifndef TEMPERATURE_EMULATION_H
#define TEMPERATURE_EMULATION_H

#include "common.h"
#include <functional>
#include <random>
#include <memory>

// Эмулятор температуры
class TemperatureEmulator
{
public:
    // Эмулятор температуры
    TemperatureEmulator(double base_temp = 20.0,
                        double amplitude = 5.0,
                        double noise_level = 0.5);

    // Получиить текущую температуру
    double getCurrentTemperature();
    // Получить текущую температуру в формате для передачи
    std::string getTemperatureAsString();
    // Установить начальную температуру
    void setBaseTemperature(double temp);
    // Установить колебания температуры (+- eps)
    void setAmplitude(double amplitude);
    // Установить шум (колебание результата)
    void setNoiseLevel(double noise_level);
    // Переключить цикл генерации температуры в течении дня
    void enableDailyCycle(bool enable);
    // Установить кастомный генератор температуры
    void setCustomGenerator(std::function<double()> generator);

private:
    // Основная температура
    double base_temperature_;
    // От основной температуры происходят отклонения в пределах амплитуды
    // Амплитуда
    double amplitude_;
    // Уровень шума
    double noise_level_;
    // Статус цикла генерации температуры в течении дня
    bool daily_cycle_enabled_;

    // Алгоритм рандома
    std::minstd_rand random_engine_;
    // Распределение шума
    std::normal_distribution<double> noise_distribution_;
    // Собственный генератор случайных значений
    std::function<double()> custom_generator_;

    // Цикл генерации температуры в течении дня
    double generateDailyCycle();
    // Генерация случайной температуры
    double generateRandomTemperature();
    // Добавить шум
    double addNoise(double temperature);
};

#endif