#include "temperature_monitor.h"
#include "time_manager.h"
#include "database.h"
#include <iostream>
#include <algorithm>
#include <numeric>

TemperatureMonitor &TemperatureMonitor::getInstance()
{
    static TemperatureMonitor instance;
    return instance;
}

bool TemperatureMonitor::initialize(const Config &config)
{
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (initialized_)
    {
        return true;
    }

    if (!Database::getInstance().initialize())
    {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    config_ = config;
    auto now = common::getCurrentTime();
    last_hourly_calculation_ = now;
    last_daily_calculation_ = now;

    auto recent_from_db = Database::getInstance().getRecentMeasurements(100);
    for (const auto &measurement : recent_from_db)
    {
        TemperatureReading reading{measurement.second, measurement.first};
        recent_measurements_.push_back(reading);
    }

    initialized_ = true;
    std::cout << "Temperature monitor initialized (HTTP mode)" << std::endl;

    return true;
}

void TemperatureMonitor::logTemperature(double temperature, const common::TimePoint &timestamp)
{
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (!initialized_)
    {
        std::cerr << "Monitor not initialized" << std::endl;
        return;
    }

    if (!Database::getInstance().logTemperature(temperature, timestamp))
    {
        std::cerr << "Failed to log temperature to database" << std::endl;
    }

    current_temperature_ = temperature;

    TemperatureReading reading{temperature, timestamp};
    recent_measurements_.push_back(reading);

    if (recent_measurements_.size() > 100)
    {
        recent_measurements_.pop_front();
    }

    addToHourlyBuffer(temperature, timestamp);
    addToDailyBuffer(temperature, timestamp);

    if (config_.console_output)
    {
        std::string time_str = common::timeToString(timestamp);
        std::cout << "[" << time_str << "] Temperature: " << temperature << "°C" << std::endl;
    }
}

void TemperatureMonitor::addToHourlyBuffer(double temperature, const common::TimePoint &timestamp)
{
    TemperatureReading reading{temperature, timestamp};
    hourly_buffer_.push_back(reading);

    auto one_hour_ago = timestamp - TimeManager::getInstance().getCustomHour();
    while (!hourly_buffer_.empty() && hourly_buffer_.front().timestamp < one_hour_ago)
    {
        hourly_buffer_.pop_front();
    }
}

void TemperatureMonitor::addToDailyBuffer(double temperature, const common::TimePoint &timestamp)
{
    TemperatureReading reading{temperature, timestamp};
    daily_buffer_.push_back(reading);

    auto one_day_ago = timestamp - TimeManager::getInstance().getCustomDay();
    while (!daily_buffer_.empty() && daily_buffer_.front().timestamp < one_day_ago)
    {
        daily_buffer_.pop_front();
    }
}

double TemperatureMonitor::getCurrentTemperature()
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    return current_temperature_;
}

double TemperatureMonitor::getHourlyAverage()
{
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto now = common::getCurrentTime();
    auto one_hour_ago = now - TimeManager::getInstance().getCustomHour();

    return Database::getInstance().getAverageInRange(one_hour_ago, now);
}

double TemperatureMonitor::getDailyAverage()
{
    std::lock_guard<std::mutex> lock(data_mutex_);

    auto now = common::getCurrentTime();
    auto one_day_ago = now - TimeManager::getInstance().getCustomDay();

    return Database::getInstance().getAverageInRange(one_day_ago, now);
}

std::vector<std::pair<common::TimePoint, double>> TemperatureMonitor::getRecentMeasurements(int count)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<std::pair<common::TimePoint, double>> result;

    int actual_count = std::min(count, static_cast<int>(recent_measurements_.size()));
    auto it = recent_measurements_.rbegin(); // Начинаем с самых свежих

    for (int i = 0; i < actual_count && it != recent_measurements_.rend(); ++i, ++it)
    {
        result.emplace_back(it->timestamp, it->temperature);
    }

    return result;
}

void TemperatureMonitor::setMeasurementInterval(std::chrono::milliseconds interval)
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    config_.measurement_interval = interval;
}

void TemperatureMonitor::shutdown()
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    Database::getInstance().shutdown();
    initialized_ = false;
    std::cout << "Temperature monitor shutdown" << std::endl;
}

TemperatureMonitor::~TemperatureMonitor()
{
    shutdown();
}