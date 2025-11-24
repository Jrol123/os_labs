#pragma once

#include "shared_data.hpp"
#include "logger.hpp"
#include <atomic>
#include <thread>
#include <chrono>

namespace cplib
{

    class Application
    {
    public:
        Application(const std::string &shared_mem_name, const std::string &log_file);
        ~Application();

        void run();

    private:
        void counterTimerThread(); // Таймер счетчика (300 мс)
        void masterCheckThread();  // Проверка состояния мастера
        void childManagerThread(); // Управление дочерними процессами
        void userInputThread();    // Пользовательский ввод
        void updateMasterStatus(); // Обновление статуса мастера

        bool launchChildProcess(int child_type); // Запуск копии

        SharedDataManager shared_data_;
        Logger logger_;
        std::atomic<bool> running_;
        std::thread counter_thread_;
        std::thread master_check_thread_;
        std::thread child_manager_thread_;
        std::thread input_thread_;
        std::chrono::milliseconds sleep_time = std::chrono::milliseconds(300);
        std::chrono::milliseconds write_time = sleep_time;
        std::chrono::milliseconds check_time = std::chrono::milliseconds(1000);
        std::chrono::milliseconds child_launch_interval = std::chrono::milliseconds(3000);

        bool is_master_;
        bool is_slave_;
        bool new_slave_status;
    };

} // namespace cplib