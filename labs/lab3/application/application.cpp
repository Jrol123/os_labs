#include "application.hpp"
#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

namespace cplib
{

    Application::Application(const std::string &shared_mem_name, const std::string &log_file)
        : shared_data_(shared_mem_name), logger_(log_file), running_(false), is_master_(false)
    {

        if (!shared_data_.isValid())
        {
            std::cerr << "Failed to create shared data!" << std::endl;
            return;
        }

        // Регистрируем подключение
        shared_data_.registerConnection();

        // Проверяем и устанавливаем статус мастера
        updateMasterStatus();
    }

    Application::~Application()
    {
        running_ = false;
        if (counter_thread_.joinable())
        {
            counter_thread_.join();
        }
        if (master_check_thread_.joinable())
        {
            master_check_thread_.join();
        }
        if (input_thread_.joinable())
        {
            input_thread_.join();
        }
    }

    void Application::updateMasterStatus()
    {
        bool new_master_status = shared_data_.isMaster();

        if (new_master_status && !is_master_)
        {
            // Стали мастером
            logger_.log("I am the new MASTER process");
            is_master_ = true;
            new_slave_status = true;
            is_slave_ = false;
        }
        else if (!new_master_status && is_master_)
        {
            // Перестали быть мастером
            logger_.log("I am no longer the MASTER process");
            is_master_ = false;
            is_slave_ = true;
        }
        else if (!is_master_ && new_slave_status)
        {
            logger_.log("I am a new SLAVE process");
            is_slave_ = true;
            new_slave_status = false;
        }
        // else if (is_master_)
        // {
        //     logger_.log("Master-alive");
        // }
        else if (is_slave_)
        {
            logger_.log("Slave-alive");
        }
    }

    void Application::run()
    {
        if (!shared_data_.isValid())
        {
            return;
        }

        running_ = true;

        // Запускаем таймер счетчика (все процессы)
        counter_thread_ = std::thread(&Application::counterTimerThread, this);

        // Запускаем проверку состояния мастера
        master_check_thread_ = std::thread(&Application::masterCheckThread, this);

        // Запускаем обработку пользовательского ввода
        userInputThread(); // В основном потоке

        running_ = false;
    }

    void Application::counterTimerThread()
    {
        while (running_)
        {
            std::this_thread::sleep_for(sleep_time);
            shared_data_.incrementCounter();

            // Только мастер пишет в лог каждую секунду (примерно 3 инкремента)
            static int increment_count = 0;
            increment_count++;
            if (is_master_ && increment_count >= 3)
            {
                logger_.logCounter(shared_data_.getCounter());
                increment_count = 0;
            }
        }
    }

    void Application::masterCheckThread()
    {
        while (running_)
        {
            std::this_thread::sleep_for(check_time); // Проверяем каждую секунду

            // Если мы не мастер, проверяем возможность стать им
            if (!is_master_)
            {
                updateMasterStatus();
            }
            else
            {
                // Если мы мастер, проверяем что мы все еще мастер
                if (!shared_data_.checkMasterAlive())
                {
                    updateMasterStatus();
                }
            }
        }
    }

    void Application::userInputThread()
    {
        std::string command;

        std::cout << "Commands: 'show' (s), 'set <value>' (m <value>), 'exit' (e, q)" << std::endl;

        while (running_)
        {
            std::cout << "> ";
            std::cin >> command;

            if (command == "show" || command == "s")
            {
                std::cout << "Counter: " << shared_data_.getCounter();
                if (is_master_)
                {
                    std::cout << " [MASTER]";
                }
                else
                {
                    std::cout << " [SLAVE]";
                }
                std::cout << std::endl;
            }
            else if (command == "set" || command == "m")
            {
                int value;
                if (std::cin >> value)
                {
                    shared_data_.setCounter(value);
                    std::cout << "Counter set to: " << value << std::endl;
                }
                else
                {
                    std::cout << "Invalid value!" << std::endl;
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                }
            }
            else if (command == "exit" || command == "e" || command == "q")
            {
                running_ = false;
                break;
            }
            else
            {
                std::cout << "Unknown command!" << std::endl;
            }
        }
    }

} // namespace cplib