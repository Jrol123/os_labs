#include "application.hpp"
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#include <tchar.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

using namespace std::chrono_literals;

namespace cplib
{

    Application::Application(const std::string &shared_mem_name, const std::string &log_file)
        : shared_data_(shared_mem_name), logger_(log_file), running_(false),
          is_master_(false), is_slave_(false), new_slave_status(true),
          running_copies_(0)
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
        if (copy_launcher_thread_.joinable())
        {
            copy_launcher_thread_.join();
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
        // else if (is_slave_)
        // {
        //     logger_.log("Slave-alive");
        // }
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

        // Если мы мастер, запускаем поток для создания копий
        if (is_master_)
        {
            copy_launcher_thread_ = std::thread(&Application::copyLauncherThread, this);
        }

        // Запускаем обработку пользовательского ввода
        userInputThread(); // В основном потоке

        running_ = false;
    }

    void Application::counterTimerThread()
    {
        auto last_log_time = std::chrono::steady_clock::now();
        auto last_master_alive_time = std::chrono::steady_clock::now();

        while (running_)
        {
            std::this_thread::sleep_for(sleep_time);
            shared_data_.incrementCounter();

            auto now = std::chrono::steady_clock::now();

            // Мастер пишет значение счетчика каждую секунду
            if (is_master_ && (now - last_log_time) >= check_time)
            {
                logger_.logCounter(shared_data_.getCounter());
                last_log_time = now;
            }

            // Мастер пишет "Master-alive" каждые 3 секунды
            // if (is_master_ && (now - last_master_alive_time) >= copy_launch_interval)
            // {
            //     logger_.log("Master-alive");
            //     last_master_alive_time = now;
            // }
        }
    }

    void Application::masterCheckThread()
    {
        while (running_)
        {
            std::this_thread::sleep_for(check_time);

            if (!is_master_)
            {
                updateMasterStatus();
                // Если стали мастером, запускаем поток для создания копий
                if (is_master_ && !copy_launcher_thread_.joinable())
                {
                    copy_launcher_thread_ = std::thread(&Application::copyLauncherThread, this);
                }
            }
            else
            {
                if (!shared_data_.checkMasterAlive())
                {
                    updateMasterStatus();
                }
            }
        }
    }

    void Application::copyLauncherThread()
    {
        auto last_launch_time = std::chrono::steady_clock::now();

        while (running_ && is_master_)
        {
            std::this_thread::sleep_for(100ms);

            auto now = std::chrono::steady_clock::now();
            if ((now - last_launch_time) >= copy_launch_interval)
            {
                // Проверяем, не запущены ли уже копии
                if (running_copies_ > 0)
                {
                    logger_.log("Previous copies still running, skipping launch");
                }
                else
                {
                    // Запускаем копии
                    if (launchCopy(1) && launchCopy(2))
                    {
                        logger_.log("Launched two copies");
                    }
                }

                last_launch_time = now;
            }
        }
    }

#if defined(_WIN32)
    bool Application::launchCopy(int copy_type)
    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // Создаем командную строку для запуска копии
        std::string cmd = "LAB.exe copy" + std::to_string(copy_type);

        char cmd_line[256];
        strcpy(cmd_line, cmd.c_str());

        // Запускаем процесс
        if (!CreateProcess(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            logger_.log("Failed to launch copy " + std::to_string(copy_type));
            return false;
        }

        running_copies_++;
        CloseHandle(pi.hThread);
        // Можно сохранить pi.hProcess для отслеживания, но для простоты будем считать что копия завершится быстро

        // В Windows сложно отслеживать завершение дочерних процессов без блокировки,
        // поэтому используем упрощенный подход - считаем что копии завершатся за 2-3 секунды
        std::thread([this, copy_type]()
                    {
            std::this_thread::sleep_for(copy_launch_interval);
            running_copies_--; })
            .detach();

        return true;
    }
#else
    bool Application::launchCopy(int copy_type)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            // Дочерний процесс - запускаем копию
            execl("./LAB", "LAB", ("copy" + std::to_string(copy_type)).c_str(), NULL);
            // Если execl вернул управление - ошибка
            exit(1);
        }
        else if (pid > 0)
        {
            // Родительский процесс
            running_copies_++;

            // Отслеживаем завершение дочернего процесса
            std::thread([this, pid, copy_type]()
                        {
                int status;
                waitpid(pid, &status, 0);
                running_copies_--; })
                .detach();

            return true;
        }
        else
        {
            logger_.log("Failed to fork copy " + std::to_string(copy_type));
            return false;
        }
    }
#endif

    bool Application::areCopiesRunning()
    {
        return running_copies_ > 0;
    }

    void Application::runAsCopy(int copy_type)
    {
        logger_.log("Copy " + std::to_string(copy_type) + " started");

        shared_data_.Lock();
        int current_counter = shared_data_.getCounter();

        if (copy_type == 1)
        {
            // Копия 1: увеличиваем на 10
            shared_data_.setCounter(current_counter + 10);
            logger_.log("Copy1: increased counter by 10, new value: " + std::to_string(current_counter + 10));
        }
        else if (copy_type == 2)
        {
            // Копия 2: умножаем на 2, ждем 2 секунды, делим на 2
            shared_data_.setCounter(current_counter * 2);
            logger_.log("Copy2: multiplied counter by 2, new value: " + std::to_string(current_counter * 2));

            shared_data_.Unlock(); // Разблокируем на время ожидания

            std::this_thread::sleep_for(2000ms);

            shared_data_.Lock();
            current_counter = shared_data_.getCounter();
            shared_data_.setCounter(current_counter / 2);
            logger_.log("Copy2: divided counter by 2, new value: " + std::to_string(current_counter / 2));
        }

        shared_data_.Unlock();
        logger_.log("Copy " + std::to_string(copy_type) + " finished");
    }

    void Application::userInputThread()
    {
        std::string command;

        std::cout << "Commands: 'show' (s), 'set <value>' (m <value>), 'exit' (e, q)" << std::endl;
        // if (is_master_)
        // {
        //     std::cout << "Status: MASTER (will launch copies every 3 seconds)" << std::endl;
        // }
        // else
        // {
        //     std::cout << "Status: SLAVE" << std::endl;
        // }

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
                    if (running_copies_ > 0)
                    {
                        std::cout << " (" << running_copies_ << " copies running)";
                    }
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