// temperature_monitor.h (упрощенная версия)
#ifndef TEMPERATURE_MONITOR_H
#define TEMPERATURE_MONITOR_H

#include "common.h"
#include <memory>
#include <mutex>
#include <vector>
#include <deque>

class TemperatureMonitor
{
public:
    struct Config
    {
        bool console_output = true;
        std::chrono::milliseconds measurement_interval = std::chrono::hours(1);

        Config() = default;
        Config(bool console, std::chrono::seconds interval = std::chrono::hours(1))
            : console_output(console), measurement_interval(interval) {}
    };

    static TemperatureMonitor &getInstance();

    bool initialize(const Config &config);
    void logTemperature(double temperature, const common::TimePoint &timestamp = common::currentTime());
    void setMeasurementInterval(std::chrono::milliseconds interval);
    void shutdown();

    // Новые методы для HTTP сервера
    double getCurrentTemperature();
    double getHourlyAverage();
    double getDailyAverage();
    std::vector<std::pair<common::TimePoint, double>> getRecentMeasurements(int count = 10);

private:
    TemperatureMonitor() = default;
    ~TemperatureMonitor();

    TemperatureMonitor(const TemperatureMonitor &) = delete;
    TemperatureMonitor &operator=(const TemperatureMonitor &) = delete;

    void addToHourlyBuffer(double temperature, const common::TimePoint &timestamp);
    void addToDailyBuffer(double temperature, const common::TimePoint &timestamp);

    Config config_;
    std::mutex data_mutex_;
    bool initialized_ = false;

    struct TemperatureReading
    {
        double temperature;
        common::TimePoint timestamp;
    };

    std::deque<TemperatureReading> hourly_buffer_;
    std::deque<TemperatureReading> daily_buffer_;
    std::deque<TemperatureReading> recent_measurements_;

    common::TimePoint last_hourly_calculation_;
    common::TimePoint last_daily_calculation_;

    double current_temperature_ = 0.0;
    double hourly_average_ = 0.0;
    double daily_average_ = 0.0;
};

#endif