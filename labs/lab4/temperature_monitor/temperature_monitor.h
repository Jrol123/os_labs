#ifndef TEMPERATURE_MONITOR_H
#define TEMPERATURE_MONITOR_H

#include "common.h"
#include "my_serial.hpp"
#include <fstream>
#include <memory>
#include <mutex>
#include <vector>
#include <deque>
#include <atomic>

// Монитор температуры
class TemperatureMonitor
{
public:
    // Конфигурация логирования
    struct Config
    {
        std::string log_directory = "logs";
        std::chrono::milliseconds measurement_interval = std::chrono::hours(1);
        bool console_output = true;
        std::string com_port = "COM2";

        // Конструктор по умолчанию
        Config() = default;

        // Конструктор с параметрами
        Config(const std::string &dir,
               std::chrono::seconds interval = std::chrono::hours(1),
               bool console = true,
               const std::string &com_port = "COM2")
            : log_directory(dir), measurement_interval(interval), console_output(console), com_port(com_port) {}
    };

    // Получить экземпляр монитора (синглтон)
    static TemperatureMonitor &getInstance();

    // Инициализация монитора - УБИРАЕМ значение по умолчанию здесь
    bool initialize(const Config &config);

    // Перегруженная версия без параметров
    bool initialize()
    {
        return initialize(Config());
    }

    // Запись измерения температуры
    void logTemperature(double temperature, const common::TimePoint &timestamp = common::currentTime());

    // Установка интервала измерений
    void setMeasurementInterval(std::chrono::milliseconds interval);

    // Остановка монитора
    void shutdown();

    bool startReadingFromCOMPort();
    void stopReadingFromCOMPort();

private:
    TemperatureMonitor() = default;
    ~TemperatureMonitor();

    // Запрет копирования
    TemperatureMonitor(const TemperatureMonitor &) = delete;
    TemperatureMonitor &operator=(const TemperatureMonitor &) = delete;

    void comPortReadingThread();

    // Ротация логов
    void rotateRawLogs();
    void rotateHourlyLogs();
    void rotateDailyLogs();

    // Вычисление средних значений
    void calculateHourlyAverage();
    void calculateDailyAverage();

    // Вспомогательные методы
    void addToHourlyBuffer(double temperature, const common::TimePoint &timestamp);
    void addToDailyBuffer(double temperature, const common::TimePoint &timestamp);
    bool hasHourPassed(const common::TimePoint &currentTime);
    bool hasDayPassed(const common::TimePoint &currentTime);
    bool hasYearPassed(const common::TimePoint &currentTime);

    // Методы для работы с кастомным временем
    std::chrono::milliseconds getHourDuration() const;
    std::chrono::milliseconds getDayDuration() const;
    std::chrono::milliseconds getYearDuration() const;

private:
    Config config_;

    bool writeToRawLog(const std::string &data);
    bool writeToHourlyLog(const std::string &data);
    bool writeToDailyLog(const std::string &data);

    // Методы для получения текущих путей к файлам
    std::string getCurrentRawLogPath() const;
    std::string getCurrentHourlyLogPath() const;
    std::string getCurrentDailyLogPath() const;

    // Пути к файлам
    std::string current_raw_log_path_;
    std::string current_hourly_log_path_;
    std::string current_daily_log_path_;

    std::mutex log_mutex_;
    bool initialized_ = false;

    std::unique_ptr<cplib::SerialPort> serial_port_;
    std::atomic<bool> com_reading_active_{false};
    std::thread com_reading_thread_;

    // Буферы для вычисления средних
    struct TemperatureReading
    {
        double temperature;
        common::TimePoint timestamp;
    };

    std::deque<TemperatureReading> hourly_buffer_; // Данные за текущий час
    std::deque<TemperatureReading> daily_buffer_;  // Данные за текущий день

    // Время последнего расчета
    common::TimePoint last_hourly_calculation_;
    common::TimePoint last_daily_calculation_;

    // Текущие даты для ротации
    std::string current_date_;
    std::string current_hour_;
};

#endif