#include "temperature_monitor.h"
#include "time_manager.h"
#include "common.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <thread>

bool TemperatureMonitor::startReadingFromCOMPort()
{
    if (!initialized_)
    {
        std::cerr << "Monitor not initialized" << std::endl;
        return false;
    }

    try
    {
        serial_port_ = std::make_unique<cplib::SerialPort>();
        cplib::SerialPort::Parameters params(cplib::SerialPort::BAUDRATE_115200);
        params.timeout = 0.0; // Неблокирующий режим
        params.parity = cplib::SerialPort::COM_PARITY_NONE;
        params.data_bits = 8;
        params.stop_bits = cplib::SerialPort::STOPBIT_ONE;
        params.controls = cplib::SerialPort::CONTROL_NONE;

        std::cout << "Trying to open COM port for reading: " << config_.com_port << std::endl;
        int result = serial_port_->Open(config_.com_port, params);
        if (result != cplib::SerialPort::RE_OK)
        {
            std::cerr << "Failed to open COM port " << config_.com_port << ", error: " << result << std::endl;
            return false;
        }

        // Устанавливаем неблокирующий режим
        serial_port_->SetTimeout(0.0);

        com_reading_active_ = true;
        com_reading_thread_ = std::thread(&TemperatureMonitor::comPortReadingThread, this);

        std::cout << "Started reading from COM port: " << config_.com_port << std::endl;
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception starting COM port reading: " << e.what() << std::endl;
        return false;
    }
}

void TemperatureMonitor::stopReadingFromCOMPort()
{
    com_reading_active_ = false;
    if (com_reading_thread_.joinable())
    {
        com_reading_thread_.join();
    }

    if (serial_port_ && serial_port_->IsOpen())
    {
        serial_port_->Close();
    }
}

void TemperatureMonitor::comPortReadingThread()
{
    std::string buffer;
    int read_count = 0;
    int empty_reads = 0;

    std::cout << "COM port reading thread started" << std::endl;

    while (com_reading_active_)
    {
        try
        {
            char read_buffer[256];
            size_t bytes_read = 0;

            int result = serial_port_->Read(read_buffer, sizeof(read_buffer) - 1, &bytes_read);

            if (result == cplib::SerialPort::RE_OK && bytes_read > 0)
            {
                empty_reads = 0; // Сброс счетчика пустых чтений
                read_buffer[bytes_read] = '\0';
                std::string new_data(read_buffer, bytes_read);
                buffer += new_data;

                std::cout << "Received " << bytes_read << " bytes: [" << new_data << "]" << std::endl;

                // Обрабатываем полные строки
                size_t pos;
                while ((pos = buffer.find('\n')) != std::string::npos)
                {
                    std::string line = buffer.substr(0, pos);
                    buffer.erase(0, pos + 1);

                    // Убираем лишние символы (CR, пробелы)
                    while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
                    {
                        line.pop_back();
                    }

                    // Парсим температуру из строки формата "TEMP:25.5"
                    if (line.find("TEMP:") == 0)
                    {
                        try
                        {
                            double temperature = std::stod(line.substr(5));
                            // std::cout << "=== PARSED TEMPERATURE: " << temperature << "°C ===" << std::endl;
                            logTemperature(temperature);
                            read_count++;
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << "Failed to parse temperature from: [" << line << "]" << std::endl;
                        }
                    }
                    else if (!line.empty())
                    {
                        std::cout << "Unknown message format: [" << line << "]" << std::endl;
                    }
                }
            }
            else
            {
                empty_reads++;
                if (empty_reads % 100 == 0)
                { // Сообщаем каждые 100 пустых чтений
                    std::cout << "Still waiting for data... (" << empty_reads << " empty reads)" << std::endl;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Exception in COM reading thread: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "COM port reading thread stopped. Total messages processed: " << read_count << std::endl;
}

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

    auto now = common::getCurrentTime();
    last_hourly_calculation_ = now;
    last_daily_calculation_ = now;
    current_date_ = common::getDateString(now);
    current_hour_ = common::getHourString(now);
    current_raw_log_path_ = getCurrentRawLogPath();
    current_hourly_log_path_ = getCurrentHourlyLogPath();
    current_daily_log_path_ = getCurrentDailyLogPath();

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

    // Начальная чистка
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

    if (hasDayPassed(timestamp) || hasHourPassed(timestamp))
    {
        // Нужно обновить файлы
        if (hasYearPassed(timestamp))
        {
            calculateDailyAverage();
            rotateDailyLogs();
        }

        if (hasDayPassed(timestamp))
        {
            calculateHourlyAverage();
            rotateRawLogs();
            rotateHourlyLogs();
        }

        current_date_ = current_date;
        current_hour_ = current_hour;
    }

    std::string time_str = common::timeToString(timestamp);
    std::string raw_data = time_str + ", " + std::to_string(temperature) + "\n";
    if (!writeToRawLog(raw_data))
    {
        std::cerr << "Failed to write to raw log" << std::endl;
    }

    addToHourlyBuffer(temperature, timestamp);
    addToDailyBuffer(temperature, timestamp);

    if (hasHourPassed(timestamp))
    {
        calculateHourlyAverage();
    }

    if (hasDayPassed(timestamp))
    {
        calculateDailyAverage();
    }

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

                // Задержка для гарантии разблокировки файла
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
    stopReadingFromCOMPort();
    shutdown();
}