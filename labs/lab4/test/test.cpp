#include "temperature_emulation.h"
#include "temperature_monitor.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <chrono>

void testCustomTimeScales()
{
    std::cout << "=== Testing Custom Time Scales ===" << std::endl;

    // Настраиваем кастомное время:
    // - 1 час = 10 секунд реального времени
    // - 1 день = 2 часа кастомного времени = 20 секунд реального времени
    // - 1 год = 2 дня кастомного времени = 40 секунд реального времени
    // - Ускорение времени: 360x (1 реальная секунда = 6 кастомных минут)

    common::setupCustomTime(
        std::chrono::seconds(10), // custom_hour: 10 секунд
        std::chrono::seconds(20), // custom_day: 20 секунд
        std::chrono::seconds(40)  // custom_year: 40 секунд
    );

    std::cout << "Custom time configured:" << std::endl;
    std::cout << "- 1 hour = 10 real seconds" << std::endl;
    std::cout << "- 1 day = 20 real seconds" << std::endl;
    std::cout << "- 1 year = 40 real seconds" << std::endl;
    std::cout << "- Time scale: 360x" << std::endl;

    TemperatureMonitor::Config config;
    config.log_directory = "logs_custom_time";
    config.measurement_interval = std::chrono::seconds(2); // Измерения каждые 2 секунды
    config.console_output = true;

    if (!TemperatureMonitor::getInstance().initialize(config))
    {
        std::cerr << "Failed to initialize temperature monitor" << std::endl;
        return;
    }

    TemperatureEmulator emulator(22.0, 3.0, 0.1);

    // Тестируем в течение 2 минут реального времени
    // В кастомном времени это будет 2 * 60 * 360 = 43200 секунд = 12 часов
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < std::chrono::minutes(2))
    {
        double temp = emulator.getCurrentTemperature();
        TemperatureMonitor::getInstance().logTemperature(temp);

        // Ждем 2 секунды реального времени (12 минут кастомного)
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Выводим текущее кастомное время для отладки
        auto custom_now = common::getCurrentTime();
        std::cout << "[Custom Time] " << common::timeToString(custom_now) << std::endl;
    }

    std::cout << "Custom time scale test completed" << std::endl;
    common::resetToRealTime();
}

void testUltraFastTime()
{
    std::cout << "\n=== Testing Ultra-Fast Time (для демонстрации ротации) ===" << std::endl;

    // Еще более быстрое время для демонстрации всех функций за несколько секунд
    common::setupCustomTime(
        std::chrono::seconds(5),  // 1 час = 5 секунд
        std::chrono::seconds(10), // 1 день = 10 секунд
        std::chrono::seconds(20)  // 1 год = 20 секунд
    );

    TemperatureMonitor::Config config;
    config.log_directory = "logs_ultrafast";
    config.measurement_interval = std::chrono::seconds(1);
    config.console_output = true;

    if (!TemperatureMonitor::getInstance().initialize(config))
    {
        std::cerr << "Failed to initialize temperature monitor" << std::endl;
        return;
    }

    TemperatureEmulator emulator(25.0, 2.0, 0.05);

    // Запускаем на 30 секунд реального времени
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(30))
    {
        double temp = emulator.getCurrentTemperature();
        TemperatureMonitor::getInstance().logTemperature(temp);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Ultra-fast time test completed" << std::endl;
    common::resetToRealTime();
}

void demonstrateTimeScales()
{
    std::cout << "\n=== Demonstrating Different Time Scales ===" << std::endl;

    // Показываем как работают разные масштабы времени
    std::vector<std::pair<std::string, double>> scales = {
        {"1x (реальное время)", 1.0},
        {"60x (1 реальная минута = 1 кастомный час)", 60.0},
        {"360x (1 реальная секунда = 6 кастомных минут)", 360.0},
        {"86400x (1 реальная секунда = 1 кастомный день)", 86400.0}};

    for (const auto &scale : scales)
    {
        std::cout << "Scale: " << scale.first << std::endl;
        common::setupCustomTime();

        auto real_start = std::chrono::steady_clock::now();
        auto custom_start = common::getCurrentTime();

        std::this_thread::sleep_for(std::chrono::seconds(2));

        auto real_end = std::chrono::steady_clock::now();
        auto custom_end = common::getCurrentTime();

        auto real_elapsed = std::chrono::duration_cast<std::chrono::seconds>(real_end - real_start);
        auto custom_elapsed = std::chrono::duration_cast<std::chrono::seconds>(custom_end - custom_start);

        std::cout << "  Real time elapsed: " << real_elapsed.count() << " seconds" << std::endl;
        std::cout << "  Custom time elapsed: " << custom_elapsed.count() << " seconds" << std::endl;
        std::cout << "  Effective scale: " << (double)custom_elapsed.count() / real_elapsed.count() << "x" << std::endl;
    }

    common::resetToRealTime();
}

int main()
{
    try
    {
        demonstrateTimeScales();
        testCustomTimeScales();
        testUltraFastTime();

        TemperatureMonitor::getInstance().shutdown();
        std::cout << "\nAll time scale tests completed successfully!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        common::resetToRealTime();
        return 1;
    }

    return 0;
}