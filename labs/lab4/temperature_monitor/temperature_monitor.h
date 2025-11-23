#ifndef TEMPERATURE_MONITOR_H
#define TEMPERATURE_MONITOR_H

#include "common.h"
#include <fstream>
#include <memory>
#include <mutex>
#include <vector>
#include <deque>

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

        // Конструктор по умолчанию
        Config() = default;

        // Конструктор с параметрами
        Config(const std::string &dir,
               std::chrono::seconds interval = std::chrono::hours(1),
               bool console = true)
            : log_directory(dir), measurement_interval(interval), console_output(console) {}
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

private:
    TemperatureMonitor() = default;
    ~TemperatureMonitor();

    // Запрет копирования
    TemperatureMonitor(const TemperatureMonitor &) = delete;
    TemperatureMonitor &operator=(const TemperatureMonitor &) = delete;

    // Открыть файлы для записи
    bool openRawLogFile();
    bool openHourlyLogFile();
    bool openDailyLogFile();

    // Закрыть файлы
    void closeRawLogFile();
    void closeHourlyLogFile();
    void closeDailyLogFile();

    // Ротация логов
    void rotateRawLogs();    // Хранит 24 часа
    void rotateHourlyLogs(); // Хранит 1 месяц
    void rotateDailyLogs();  // Хранит текущий год

    // Вычисление средних значений
    void calculateHourlyAverage();
    void calculateDailyAverage();

    // Вспомогательные методы
    void addToHourlyBuffer(double temperature, const common::TimePoint &timestamp);
    void addToDailyBuffer(double temperature, const common::TimePoint &timestamp);
    bool shouldCalculateHourlyAverage(const common::TimePoint &currentTime);
    bool shouldCalculateDailyAverage(const common::TimePoint &currentTime);

    // Методы для работы с кастомным временем
    std::chrono::milliseconds getHourDuration() const;
    std::chrono::milliseconds getDayDuration() const; 
    std::chrono::milliseconds getYearDuration() const;

private:
    Config config_;

    // Файловые потоки
    std::ofstream raw_log_file_;
    std::ofstream hourly_log_file_;
    std::ofstream daily_log_file_;

    std::string current_raw_log_path_;
    std::string current_hourly_log_path_;
    std::string current_daily_log_path_;

    std::mutex log_mutex_;
    bool initialized_ = false;

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