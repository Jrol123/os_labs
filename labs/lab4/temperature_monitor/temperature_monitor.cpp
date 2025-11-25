#include "temperature_monitor.h"
#include "time_manager.h"
#include "common.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <thread>

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
    current_raw_log_path_ = getCurrentRawLogPath();
    current_hourly_log_path_ = getCurrentHourlyLogPath();
    current_daily_log_path_ = getCurrentDailyLogPath();

    // Создаем начальные записи в файлах
    std::string header_raw = "# Raw Temperature Measurements\n# Created: " +
                             common::timeToString(now) + "\n# Format: Timestamp, Temperature\n";
    std::string header_hourly = "# Hourly Average Temperature\n# Created: " +
                                common::timeToString(now) + "\n# Format: Timestamp, AverageTemperature\n";
    std::string header_daily = "# Daily Average Temperature\n# Created: " +
                               common::timeToString(now) + "\n# Format: Timestamp, AverageTemperature\n";

    if (!writeToRawLog(header_raw) || !writeToHourlyLog(header_hourly) || !writeToDailyLog(header_daily))
    {
        std::cerr << "Failed to write initial headers to log files" << std::endl;
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

bool TemperatureMonitor::hasHourPassed(const common::TimePoint &currentTime)
{
    auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - last_hourly_calculation_);
    return time_since_last >= getHourDuration();
}

bool TemperatureMonitor::hasDayPassed(const common::TimePoint &currentTime)
{
    auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - last_daily_calculation_);
    return time_since_last >= getDayDuration();
}

bool TemperatureMonitor::hasYearPassed(const common::TimePoint &currentTime)
{
    auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - last_daily_calculation_);
    return time_since_last >= getYearDuration();
}

std::string TemperatureMonitor::getCurrentRawLogPath() const
{
    auto now = common::getCurrentTime();
    std::string filename = "raw_temperature_" + common::timeToFileName(now) + ".txt";
    return config_.log_directory + PATH_SEPARATOR + filename;
}

std::string TemperatureMonitor::getCurrentHourlyLogPath() const
{
    auto now = common::getCurrentTime();
    std::string filename = "hourly_average_" + common::getDateString(now) + ".txt";
    return config_.log_directory + PATH_SEPARATOR + filename;
}

std::string TemperatureMonitor::getCurrentDailyLogPath() const
{
    auto now = common::getCurrentTime();
    std::string filename = "daily_average_" + common::getDateString(now).substr(0, 6) + ".txt";
    return config_.log_directory + PATH_SEPARATOR + filename;
}

bool TemperatureMonitor::writeToRawLog(const std::string &data)
{
    std::string file_path = current_raw_log_path_;
    std::ofstream file(file_path, std::ios::app);
    if (!file.is_open())
    {
        std::cerr << "Cannot open raw log file for writing: " << file_path << std::endl;
        return false;
    }
    file << data;
    file.flush();
    return true;
}

bool TemperatureMonitor::writeToHourlyLog(const std::string &data)
{
    std::string file_path = current_hourly_log_path_;
    std::ofstream file(file_path, std::ios::app);
    if (!file.is_open())
    {
        std::cerr << "Cannot open hourly log file for writing: " << file_path << std::endl;
        return false;
    }
    file << data;
    file.flush();
    return true;
}

bool TemperatureMonitor::writeToDailyLog(const std::string &data)
{
    std::string file_path = current_daily_log_path_;
    std::ofstream file(file_path, std::ios::app);
    if (!file.is_open())
    {
        std::cerr << "Cannot open daily log file for writing: " << file_path << std::endl;
        return false;
    }
    file << data;
    file.flush();
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

    std::cout << "CD: " << current_date << " AND CD_: " << current_date_ << std::endl;

    if (hasDayPassed(timestamp) || hasHourPassed(timestamp))
    {
        // Нужно обновить файлы
        if (hasYearPassed(timestamp))
        {
            calculateDailyAverage(); // Рассчитываем среднее за предыдущий день
            rotateDailyLogs();
        }

        if (hasDayPassed(timestamp))
        {
            calculateHourlyAverage(); // Рассчитываем среднее за предыдущий час
            rotateRawLogs();
            rotateHourlyLogs();
        }

        current_date_ = current_date;
        current_hour_ = current_hour;
    }

    // Записываем в raw лог
    std::string time_str = common::timeToString(timestamp);
    std::string raw_data = time_str + ", " + std::to_string(temperature) + "\n";
    if (!writeToRawLog(raw_data))
    {
        std::cerr << "Failed to write to raw log" << std::endl;
    }

    // Добавляем в буферы для средних
    addToHourlyBuffer(temperature, timestamp);
    addToDailyBuffer(temperature, timestamp);

    // Проверяем нужно ли рассчитывать средние
    if (hasHourPassed(timestamp))
    {
        calculateHourlyAverage();
    }

    if (hasDayPassed(timestamp))
    {
        calculateDailyAverage();
    }

    // Вывод в консоль если включен
    if (config_.console_output)
    {
        std::cout << "[" << time_str << "] Temperature: " << temperature << "°C" << std::endl;
    }
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
    std::string hourly_data = time_str + ", " + std::to_string(average) + "\n";

    if (!writeToHourlyLog(hourly_data))
    {
        std::cerr << "Failed to write hourly average" << std::endl;
    }

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
    std::string daily_data = time_str + ", " + std::to_string(average) + "\n";

    if (!writeToDailyLog(daily_data))
    {
        std::cerr << "Failed to write daily average" << std::endl;
    }

    last_daily_calculation_ = timestamp;

    if (config_.console_output)
    {
        std::cout << "[DAILY_AVG] Average temperature: " << average << "°C" << std::endl;
    }
}

void TemperatureMonitor::rotateRawLogs()
{
    auto files = common::getFilesInDirectory(config_.log_directory);
    auto now = common::getCurrentTime();

    std::cout << "=== rotateRawLogs START ===" << std::endl;
    std::cout << "Current time: " << common::timeToString(now) << std::endl;
    std::cout << "Day duration: " << getDayDuration().count() << " ms" << std::endl;
    std::cout << "Files in directory: " << files.size() << std::endl;

    for (const auto &file : files)
    {
        std::cout << "Processing file: " << file << std::endl;

        if (file.find("raw_temperature_") == 0)
        {
            auto file_time = common::parseTimeFromFileName(file);
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - file_time);

            std::cout << "  File time: " << common::timeToString(file_time) << std::endl;
            std::cout << "  Age: " << age.count() << " ms" << std::endl;
            std::cout << "  Day duration: " << getDayDuration().count() << " ms" << std::endl;
            std::cout << "  Should delete: " << (age > getDayDuration() ? "YES" : "NO") << std::endl;

            if (age > getDayDuration())
            {
                std::cout << "  DELETING FILE: " << file << std::endl;
                std::string full_path = config_.log_directory + PATH_SEPARATOR + file;

                // Добавляем небольшую задержку для гарантии разблокировки файла
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                if (common::deleteFile(full_path))
                {
                    std::cout << "  SUCCESS: Deleted old raw log: " << file << std::endl;
                }
                else
                {
                    std::cout << "  FAILED: Could not delete file: " << file << std::endl;
                }
            }
        }
    }

    std::cout << "=== rotateRawLogs END ===" << std::endl;
}

void TemperatureMonitor::rotateHourlyLogs()
{
    auto files = common::getFilesInDirectory(config_.log_directory);
    auto now = common::getCurrentTime();

    std::cout << "=== rotateHourlyLogs START ===" << std::endl;
    std::cout << "Month duration: " << (getDayDuration() * 30).count() << " ms" << std::endl;

    for (const auto &file : files)
    {
        if (file.find("hourly_average_") == 0)
        {
            auto file_time = common::parseTimeFromFileName(file);
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - file_time);

            std::cout << "File: " << file << ", Age: " << age.count() << " ms" << std::endl;

            if (age > (getDayDuration() * 30))
            {
                std::cout << "DELETING hourly file: " << file << std::endl;
                std::string full_path = config_.log_directory + PATH_SEPARATOR + file;

                // Задержка для гарантии разблокировки файла
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                if (common::deleteFile(full_path))
                {
                    std::cout << "Deleted old hourly log: " << file << std::endl;
                }
                else
                {
                    std::cout << "FAILED to delete hourly log: " << file << std::endl;
                }
            }
        }
    }

    std::cout << "=== rotateHourlyLogs END ===" << std::endl;
}

void TemperatureMonitor::rotateDailyLogs()
{
    auto files = common::getFilesInDirectory(config_.log_directory);
    auto now = common::getCurrentTime();

    std::cout << "=== rotateDailyLogs START ===" << std::endl;
    std::cout << "Year duration: " << getYearDuration().count() << " ms" << std::endl;

    for (const auto &file : files)
    {
        if (file.find("daily_average_") == 0)
        {
            auto file_time = common::parseTimeFromFileName(file);
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - file_time);

            std::cout << "File: " << file << ", Age: " << age.count() << " ms" << std::endl;

            if (age > getYearDuration())
            {
                std::cout << "DELETING daily file: " << file << std::endl;
                std::string full_path = config_.log_directory + PATH_SEPARATOR + file;

                // Задержка для гарантии разблокировки файл
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                if (common::deleteFile(full_path))
                {
                    std::cout << "Deleted old daily log: " << file << std::endl;
                }
                else
                {
                    std::cout << "FAILED to delete daily log: " << file << std::endl;
                }
            }
        }
    }

    std::cout << "=== rotateDailyLogs END ===" << std::endl;
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

    initialized_ = false;
    std::cout << "Temperature monitor shutdown" << std::endl;
}

TemperatureMonitor::~TemperatureMonitor()
{
    shutdown();
}