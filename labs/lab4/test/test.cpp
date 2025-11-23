#include "temperature_emulation.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

int main() {
    // Создаем эмулятор с базовой температурой 20°C, 
    // амплитудой 5°C и шумом 0.2°C
    TemperatureEmulator emulator(20.0, 5.0, 0.2);
    
    std::cout << "Эмулятор температуры запущен:\n";
    std::cout << std::fixed << std::setprecision(2);
    
    // Тест на 10 измерений с интервалом 1 секунда
    for (int i = 0; i < 10; ++i) {
        double temp = emulator.getCurrentTemperature();
        std::cout << "Измерение " << (i + 1) << ": " << temp << "°C\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Тест с отключенным суточным циклом
    std::cout << "\nСлучайная температура:\n";
    emulator.enableDailyCycle(false);
    
    for (int i = 0; i < 5; ++i) {
        double temp = emulator.getCurrentTemperature();
        std::cout << "Случайное измерение " << (i + 1) << ": " << temp << "°C\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // Тест с кастомным генератором
    std::cout << "\nКастомный генератор (линейное увеличение):\n";
    double custom_temp = 15.0;
    emulator.setCustomGenerator([&custom_temp]() {
        custom_temp += 0.5;
        return custom_temp;
    });
    
    for (int i = 0; i < 5; ++i) {
        double temp = emulator.getCurrentTemperature();
        std::cout << "Кастомное измерение " << (i + 1) << ": " << temp << "°C\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    return 0;
}