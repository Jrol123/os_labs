#ifndef TEMPERATURE_MONITOR_H
#define TEMPERATURE_MONITOR_H

#include "../common/common.h"
#include <fstream>
#include <memory>
#include <mutex>

class TemperatureMonitor
{
public:
    // Конфигурация логирования
    struct Config
    {
        std::string log_directory = "logs";
        std::chrono::seconds measurement_interval = std::chrono::hours(1);
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
    void setMeasurementInterval(std::chrono::seconds interval);

    // Остановка монитора
    void shutdown();

private:
    TemperatureMonitor() = default;
    ~TemperatureMonitor();

    // Запрет копирования
    TemperatureMonitor(const TemperatureMonitor &) = delete;
    TemperatureMonitor &operator=(const TemperatureMonitor &) = delete;

    // Открыть файл для записи
    bool openLogFile();

    // Закрыть файл
    void closeLogFile();

    // Ротация логов (будет реализована позже)
    void rotateLogsIfNeeded();

private:
    Config config_;
    std::ofstream log_file_;
    std::string current_log_file_path_;
    std::mutex log_mutex_;
    bool initialized_ = false;
};

#endif