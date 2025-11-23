#ifndef TEMPERATURE_MONITOR_H
#define TEMPERATURE_MONITOR_H

#include "common.h"
#include <fstream>
#include <memory>
#include <mutex>

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

    // Открыть файл для записи
    bool openLogFile();

    // Закрыть файл
    void closeLogFile();

    // Ротация логов (будет реализована позже)
    void rotateLogsIfNeeded();

private:
    // Конфиг логгирования
    Config config_;
    // Файл логгирования
    std::ofstream log_file_;
    // Текущий путь к файлу логгирования
    std::string current_log_file_path_;
    std::mutex log_mutex_;
    // Статус инициализации
    bool initialized_ = false;
};

#endif