#pragma once

#include "shared_data.hpp"
#include "logger.hpp"
#include <atomic>
#include <thread>
#include <chrono>

namespace cplib {

class Application {
public:
    Application(const std::string& shared_mem_name, const std::string& log_file);
    ~Application();
    
    void run();

private:
    void counterTimerThread();     // Таймер счетчика (300 мс)
    void userInputThread();        // Пользовательский ввод
    
    SharedDataManager shared_data_;
    Logger logger_;
    std::atomic<bool> running_;
    std::thread counter_thread_;
    std::thread input_thread_;
    bool is_master_;
};

} // namespace cplib