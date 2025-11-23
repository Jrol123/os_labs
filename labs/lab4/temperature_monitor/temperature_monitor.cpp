#include "temperature_monitor.h"
#include "time_manager.h"
#include "common.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>

std::chrono::milliseconds TemperatureMonitor::getHourDuration() const
{
    return TimeManager::getInstance().getCustomHour();
}

std::chrono::milliseconds TemperatureMonitor::getDayDuration() const
{
    return TimeManager::getInstance().getCustomDay();
}

std::chrono::milliseconds TemperatureMonitor::getYearDuration() const
{
    return TimeManager::getInstance().getCustomYear();
}

TemperatureMonitor &TemperatureMonitor::getInstance()
{
    static TemperatureMonitor instance;
    return instance;
}

bool TemperatureMonitor::initialize(const Config &config)
{
    std::lock_guard<std::mutex> lock(log_mutex_);

    if (initialized_)
    {
        return true;
    }

    config_ = config;

    if (!common::createDirectory(config_.log_directory))
    {
        std::cerr << "Failed to create log directory: " << config_.log_directory << std::endl;
        return false;
    }

    // Инициализация времени
    auto now = common::getCurrentTime();
    last_hourly_calculation_ = now;
    last_daily_calculation_ = now;
    current_date_ = common::getDateString(now);
    current_hour_ = common::getHourString(now);

    // Открываем файлы
    if (!openRawLogFile() || !openHourlyLogFile() || !openDailyLogFile())
    {
        std::cerr << "Failed to open log files" << std::endl;
        return false;
    }

    // Выполняем первоначальную ротацию
    rotateRawLogs();
    rotateHourlyLogs();
    rotateDailyLogs();

    initialized_ = true;
    std::cout << "Temperature monitor initialized. Log directory: " << config_.log_directory << std::endl;

    return true;
}

bool TemperatureMonitor::openRawLogFile()
{
    closeRawLogFile();

    auto now = common::getCurrentTime();
    std::string filename = "raw_temperature_" + common::timeToFileName(now) + ".txt";
    current_raw_log_path_ = config_.log_directory + PATH_SEPARATOR + filename;

    raw_log_file_.open(current_raw_log_path_, std::ios::app);
    if (!raw_log_file_.is_open())
    {
        std::cerr << "Cannot open raw log file: " << current_raw_log_path_ << std::endl;
        return false;
    }

    raw_log_file_ << "# Raw Temperature Measurements" << std::endl;
    raw_log_file_ << "# Created: " << common::timeToString(now) << std::endl;
    raw_log_file_ << "# Format: Timestamp, Temperature" << std::endl;
    raw_log_file_.flush();

    return true;
}

bool TemperatureMonitor::openHourlyLogFile()
{
    closeHourlyLogFile();

    auto now = common::getCurrentTime();
    std::string filename = "hourly_average_" + common::getDateString(now) + ".txt";
    current_hourly_log_path_ = config_.log_directory + PATH_SEPARATOR + filename;

    hourly_log_file_.open(current_hourly_log_path_, std::ios::app);
    if (!hourly_log_file_.is_open())
    {
        std::cerr << "Cannot open hourly log file: " << current_hourly_log_path_ << std::endl;
        return false;
    }

    hourly_log_file_ << "# Hourly Average Temperature" << std::endl;
    hourly_log_file_ << "# Created: " << common::timeToString(now) << std::endl;
    hourly_log_file_ << "# Format: Timestamp, AverageTemperature" << std::endl;
    hourly_log_file_.flush();

    return true;
}

bool TemperatureMonitor::openDailyLogFile()
{
    closeDailyLogFile();

    auto now = common::getCurrentTime();
    std::string filename = "daily_average_" + common::getDateString(now).substr(0, 6) + ".txt"; // YYYYMM
    current_daily_log_path_ = config_.log_directory + PATH_SEPARATOR + filename;

    daily_log_file_.open(current_daily_log_path_, std::ios::app);
    if (!daily_log_file_.is_open())
    {
        std::cerr << "Cannot open daily log file: " << current_daily_log_path_ << std::endl;
        return false;
    }

    daily_log_file_ << "# Daily Average Temperature" << std::endl;
    daily_log_file_ << "# Created: " << common::timeToString(now) << std::endl;
    daily_log_file_ << "# Format: Timestamp, AverageTemperature" << std::endl;
    daily_log_file_.flush();

    return true;
}

void TemperatureMonitor::logTemperature(double temperature, const common::TimePoint &timestamp)
{
    std::lock_guard<std::mutex> lock(log_mutex_);

    if (!initialized_)
    {
        std::cerr << "Monitor not initialized" << std::endl;
        return;
    }

    // Проверяем ротацию файлов
    std::string current_date = common::getDateString(timestamp);
    std::string current_hour = common::getHourString(timestamp);

    if (current_date != current_date_ || current_hour != current_hour_)
    {
        // Нужно обновить файлы
        if (current_date != current_date_)
        {
            calculateDailyAverage(); // Рассчитываем среднее за предыдущий день
            openDailyLogFile();
            rotateDailyLogs();
        }

        if (current_hour != current_hour_)
        {
            calculateHourlyAverage(); // Рассчитываем среднее за предыдущий час
            openRawLogFile();
            openHourlyLogFile();
            rotateRawLogs();
            rotateHourlyLogs();
        }

        current_date_ = current_date;
        current_hour_ = current_hour;
    }

    // Записываем в raw лог
    std::string time_str = common::timeToString(timestamp);
    raw_log_file_ << time_str << ", " << std::fixed << std::setprecision(2) << temperature << std::endl;
    raw_log_file_.flush();

    // Добавляем в буферы для средних
    addToHourlyBuffer(temperature, timestamp);
    addToDailyBuffer(temperature, timestamp);

    // Проверяем нужно ли рассчитывать средние
    if (shouldCalculateHourlyAverage(timestamp))
    {
        calculateHourlyAverage();
    }

    if (shouldCalculateDailyAverage(timestamp))
    {
        calculateDailyAverage();
    }

    // Вывод в консоль если включен
    if (config_.console_output)
    {
        std::cout << "[" << time_str << "] Temperature: " << temperature << "°C" << std::endl;
    }
}

void TemperatureMonitor::addToHourlyBuffer(double temperature, const common::TimePoint &timestamp)
{
    TemperatureReading reading{temperature, timestamp};
    hourly_buffer_.push_back(reading);

    auto one_hour_ago = timestamp - getHourDuration();
    while (!hourly_buffer_.empty() && hourly_buffer_.front().timestamp < one_hour_ago)
    {
        hourly_buffer_.pop_front();
    }
}

void TemperatureMonitor::addToDailyBuffer(double temperature, const common::TimePoint &timestamp)
{
    TemperatureReading reading{temperature, timestamp};
    daily_buffer_.push_back(reading);

    auto one_day_ago = timestamp - getDayDuration();
    while (!daily_buffer_.empty() && daily_buffer_.front().timestamp < one_day_ago)
    {
        daily_buffer_.pop_front();
    }
}

bool TemperatureMonitor::shouldCalculateHourlyAverage(const common::TimePoint &currentTime)
{
    auto time_since_last = std::chrono::duration_cast<std::chrono::minutes>(
        currentTime - last_hourly_calculation_);
    return time_since_last >= std::chrono::minutes(60);
}

bool TemperatureMonitor::shouldCalculateDailyAverage(const common::TimePoint &currentTime)
{
    auto time_since_last = std::chrono::duration_cast<std::chrono::hours>(
        currentTime - last_daily_calculation_);
    return time_since_last >= std::chrono::hours(24);
}

void TemperatureMonitor::calculateHourlyAverage()
{
    if (hourly_buffer_.empty())
    {
        return;
    }

    double sum = 0.0;
    for (const auto &reading : hourly_buffer_)
    {
        sum += reading.temperature;
    }

    double average = sum / hourly_buffer_.size();
    auto timestamp = common::getCurrentTime();

    std::string time_str = common::timeToString(timestamp);
    hourly_log_file_ << time_str << ", " << std::fixed << std::setprecision(2) << average << std::endl;
    hourly_log_file_.flush();

    last_hourly_calculation_ = timestamp;

    if (config_.console_output)
    {
        std::cout << "[HOURLY_AVG] Average temperature: " << average << "°C" << std::endl;
    }
}

void TemperatureMonitor::calculateDailyAverage()
{
    if (daily_buffer_.empty())
    {
        return;
    }

    double sum = 0.0;
    for (const auto &reading : daily_buffer_)
    {
        sum += reading.temperature;
    }

    double average = sum / daily_buffer_.size();
    auto timestamp = common::getCurrentTime();

    std::string time_str = common::timeToString(timestamp);
    daily_log_file_ << time_str << ", " << std::fixed << std::setprecision(2) << average << std::endl;
    daily_log_file_.flush();

    last_daily_calculation_ = timestamp;

    if (config_.console_output)
    {
        std::cout << "[DAILY_AVG] Average temperature: " << average << "°C" << std::endl;
    }
}

void TemperatureMonitor::rotateRawLogs()
{
    auto files = common::getFilesInDirectory(config_.log_directory);
    auto now = common::getCurrentTime(); // Используем кастомное время

    for (const auto &file : files)
    {
        if (file.find("raw_temperature_") == 0)
        {
            auto file_time = common::parseTimeFromFileName(file);
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - file_time);

            // Используем кастомную длительность дня
            if (age > getDayDuration())
            {
                std::string full_path = config_.log_directory + PATH_SEPARATOR + file;
                if (common::deleteFile(full_path))
                {
                    std::cout << "Deleted old raw log: " << file << std::endl;
                }
            }
        }
    }
}

void TemperatureMonitor::rotateHourlyLogs()
{
    auto files = common::getFilesInDirectory(config_.log_directory);
    auto now = common::getCurrentTime(); // Используем кастомное время

    for (const auto &file : files)
    {
        if (file.find("hourly_average_") == 0)
        {
            auto file_time = common::parseTimeFromFileName(file);
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - file_time);

            // Используем кастомную длительность месяца (30 дней)
            if (age > (getDayDuration() * 30))
            {
                std::string full_path = config_.log_directory + PATH_SEPARATOR + file;
                if (common::deleteFile(full_path))
                {
                    std::cout << "Deleted old hourly log: " << file << std::endl;
                }
            }
        }
    }
}

void TemperatureMonitor::rotateDailyLogs()
{
    auto files = common::getFilesInDirectory(config_.log_directory);
    auto now = common::getCurrentTime(); // Используем кастомное время

    for (const auto &file : files)
    {
        if (file.find("daily_average_") == 0)
        {
            auto file_time = common::parseTimeFromFileName(file);
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - file_time);

            // Используем кастомную длительность года
            if (age > getYearDuration())
            {
                std::string full_path = config_.log_directory + PATH_SEPARATOR + file;
                if (common::deleteFile(full_path))
                {
                    std::cout << "Deleted old daily log: " << file << std::endl;
                }
            }
        }
    }
}
void TemperatureMonitor::closeRawLogFile()
{
    if (raw_log_file_.is_open())
    {
        raw_log_file_.close();
    }
}

void TemperatureMonitor::closeHourlyLogFile()
{
    if (hourly_log_file_.is_open())
    {
        hourly_log_file_.close();
    }
}

void TemperatureMonitor::closeDailyLogFile()
{
    if (daily_log_file_.is_open())
    {
        daily_log_file_.close();
    }
}

void TemperatureMonitor::setMeasurementInterval(std::chrono::milliseconds interval)
{
    std::lock_guard<std::mutex> lock(log_mutex_);
    config_.measurement_interval = interval;
}

void TemperatureMonitor::shutdown()
{
    std::lock_guard<std::mutex> lock(log_mutex_);

    // Рассчитываем финальные средние
    calculateHourlyAverage();
    calculateDailyAverage();

    closeRawLogFile();
    closeHourlyLogFile();
    closeDailyLogFile();

    initialized_ = false;
    std::cout << "Temperature monitor shutdown" << std::endl;
}

TemperatureMonitor::~TemperatureMonitor()
{
    shutdown();
}