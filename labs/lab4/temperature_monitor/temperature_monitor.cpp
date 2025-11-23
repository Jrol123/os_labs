#include "temperature_monitor.h"
#include "common.h"
#include <iostream>
#include <iomanip>

TemperatureMonitor& TemperatureMonitor::getInstance() {
    static TemperatureMonitor instance;
    return instance;
}

bool TemperatureMonitor::initialize(const Config& config) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (initialized_) {
        return true; // Уже инициализирован
    }
    
    config_ = config;
    
    // Создаем директорию для логов
    if (!common::createDirectory(config_.log_directory)) {
        std::cerr << "Failed to create log directory: " << config_.log_directory << std::endl;
        return false;
    }
    
    // Открываем файл для записи
    if (!openLogFile()) {
        std::cerr << "Failed to open log file" << std::endl;
        return false;
    }
    
    initialized_ = true;
    std::cout << "Temperature logger initialized. Log directory: " << config_.log_directory << std::endl;
    
    return true;
}

bool TemperatureMonitor::openLogFile() {
    auto now = common::currentTime();
    std::string filename = "temperature_log_" + common::timeToFileName(now) + ".txt";
    current_log_file_path_ = config_.log_directory + PATH_SEPARATOR + filename;
    
    log_file_.open(current_log_file_path_, std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "Cannot open log file: " << current_log_file_path_ << std::endl;
        return false;
    }
    
    // Записываем заголовок
    log_file_ << "# Temperature Log File" << std::endl;
    log_file_ << "# Created: " << common::timeToString(now) << std::endl;
    log_file_ << "# Format: Timestamp, Temperature" << std::endl;
    log_file_.flush();
    
    return true;
}

void TemperatureMonitor::logTemperature(double temperature, const common::TimePoint& timestamp) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (!initialized_ || !log_file_.is_open()) {
        std::cerr << "Logger not initialized or file not open" << std::endl;
        return;
    }
    
    std::string time_str = common::timeToString(timestamp);
    
    // Запись в файл
    log_file_ << time_str << ", " << std::fixed << std::setprecision(2) << temperature << std::endl;
    log_file_.flush();
    
    // Вывод в консоль если включен
    if (config_.console_output) {
        std::cout << "[" << time_str << "] Temperature: " << temperature << "°C" << std::endl;
    }
    
    // Проверяем ротацию логов (будет реализована позже)
    rotateLogsIfNeeded();
}

void TemperatureMonitor::setMeasurementInterval(std::chrono::milliseconds interval) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    config_.measurement_interval = interval;
}

void TemperatureMonitor::rotateLogsIfNeeded() {
    // TODO: Реализовать ротацию логов
    // Пока просто оставляем пустым
}

void TemperatureMonitor::closeLogFile() {
    if (log_file_.is_open()) {
        log_file_ << "# Log file closed: " << common::timeToString(common::currentTime()) << std::endl;
        log_file_.close();
    }
}

void TemperatureMonitor::shutdown() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    closeLogFile();
    initialized_ = false;
}

TemperatureMonitor::~TemperatureMonitor() {
    shutdown();
}