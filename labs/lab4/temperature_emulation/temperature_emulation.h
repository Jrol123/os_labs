#ifndef TEMPERATURE_EMULATION_H
#define TEMPERATURE_EMULATION_H

#include "common.h"
#include "my_serial.hpp"
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

    // Получить текущую температуру
    double getCurrentTemperature();
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

    // Инициализировать COM порт
    bool initializeCOMPort(const std::string& port_name = "COM1");
    // Отправить температуру в порт
    void sendTemperatureToPort(double temperature);
    // 
    void closeCOMPort();

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

    // COM-порт для передачи данных
    std::unique_ptr<cplib::SerialPort> serial_port_;
    // Статус COM-порта
    bool com_initialized_ = false;

    // Цикл генерации температуры в течении дня
    double generateDailyCycle();
    // Генерация случайной температуры
    double generateRandomTemperature();
    // Добавить шум
    double addNoise(double temperature);
};

#endif