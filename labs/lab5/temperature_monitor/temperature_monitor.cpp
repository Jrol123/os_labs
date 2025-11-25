// temperature_monitor.cpp (упрощенная версия)
#include "temperature_monitor.h"
#include "time_manager.h"
#include <iostream>
#include <algorithm>
#include <numeric>

TemperatureMonitor& TemperatureMonitor::getInstance() {
    static TemperatureMonitor instance;
    return instance;
}

bool TemperatureMonitor::initialize(const Config& config) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (initialized_) {
        return true;
    }

    config_ = config;
    auto now = common::getCurrentTime();
    last_hourly_calculation_ = now;
    last_daily_calculation_ = now;

    initialized_ = true;
    std::cout << "Temperature monitor initialized (HTTP mode)" << std::endl;

    return true;
}

void TemperatureMonitor::logTemperature(double temperature, const common::TimePoint& timestamp) {
    std::lock_guard<std::mutex> lock(data_mutex_);

    if (!initialized_) {
        std::cerr << "Monitor not initialized" << std::endl;
        return;
    }

    // Обновляем текущую температуру
    current_temperature_ = temperature;

    // Сохраняем в список последних измерений
    TemperatureReading reading{temperature, timestamp};
    recent_measurements_.push_back(reading);
    
    // Ограничиваем размер буфера
    if (recent_measurements_.size() > 100) {
        recent_measurements_.pop_front();
    }

    addToHourlyBuffer(temperature, timestamp);
    addToDailyBuffer(temperature, timestamp);

    if (config_.console_output) {
        std::string time_str = common::timeToString(timestamp);
        std::cout << "[" << time_str << "] Temperature: " << temperature << "°C" << std::endl;
    }
}

void TemperatureMonitor::addToHourlyBuffer(double temperature, const common::TimePoint& timestamp) {
    TemperatureReading reading{temperature, timestamp};
    hourly_buffer_.push_back(reading);

    auto one_hour_ago = timestamp - TimeManager::getInstance().getCustomHour();
    while (!hourly_buffer_.empty() && hourly_buffer_.front().timestamp < one_hour_ago) {
        hourly_buffer_.pop_front();
    }
}

void TemperatureMonitor::addToDailyBuffer(double temperature, const common::TimePoint& timestamp) {
    TemperatureReading reading{temperature, timestamp};
    daily_buffer_.push_back(reading);

    auto one_day_ago = timestamp - TimeManager::getInstance().getCustomDay();
    while (!daily_buffer_.empty() && daily_buffer_.front().timestamp < one_day_ago) {
        daily_buffer_.pop_front();
    }
}

double TemperatureMonitor::getCurrentTemperature() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return current_temperature_;
}

double TemperatureMonitor::getHourlyAverage() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (hourly_buffer_.empty()) return 0.0;
    
    double sum = 0.0;
    for (const auto& reading : hourly_buffer_) {
        sum += reading.temperature;
    }
    return sum / hourly_buffer_.size();
}

double TemperatureMonitor::getDailyAverage() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (daily_buffer_.empty()) return 0.0;
    
    double sum = 0.0;
    for (const auto& reading : daily_buffer_) {
        sum += reading.temperature;
    }
    return sum / daily_buffer_.size();
}

std::vector<std::pair<common::TimePoint, double>> TemperatureMonitor::getRecentMeasurements(int count) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    std::vector<std::pair<common::TimePoint, double>> result;
    
    int actual_count = std::min(count, static_cast<int>(recent_measurements_.size()));
    auto it = recent_measurements_.rbegin(); // Начинаем с самых свежих
    
    for (int i = 0; i < actual_count && it != recent_measurements_.rend(); ++i, ++it) {
        result.emplace_back(it->timestamp, it->temperature);
    }
    
    return result;
}

void TemperatureMonitor::setMeasurementInterval(std::chrono::milliseconds interval) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    config_.measurement_interval = interval;
}

void TemperatureMonitor::shutdown() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    initialized_ = false;
    std::cout << "Temperature monitor shutdown" << std::endl;
}

TemperatureMonitor::~TemperatureMonitor() {
    shutdown();
}