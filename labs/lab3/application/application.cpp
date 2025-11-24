#include "application.hpp"
#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

namespace cplib
{

    Application::Application(const std::string &shared_mem_name, const std::string &log_file)
        : shared_data_(shared_mem_name), logger_(log_file), running_(false), is_master_(false),
          is_slave_(false), new_slave_status(true)
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
        if (child_manager_thread_.joinable())
        {
            child_manager_thread_.join();
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

        // Только мастер запускает управление дочерними процессами
        if (is_master_)
        {
            child_manager_thread_ = std::thread(&Application::childManagerThread, this);
        }

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

            static auto timestamp = std::chrono::system_clock::now();
            auto curr_timestamp = std::chrono::system_clock::now();
            if (is_master_ && curr_timestamp - timestamp >= write_time)
            {
                logger_.logCounter(shared_data_.getCounter());
                timestamp = curr_timestamp;
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
                // Если стали мастером, запускаем управление дочерними процессами
                if (is_master_ && !child_manager_thread_.joinable())
                {
                    child_manager_thread_ = std::thread(&Application::childManagerThread, this);
                }
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

    void Application::childManagerThread()
    {
        while (running_ && is_master_)
        {
            std::this_thread::sleep_for(child_launch_interval); // Раз в 3 секунды

            // Проверяем, завершились ли предыдущие копии
            int child1_pid = shared_data_.getChild1Pid();
            int child2_pid = shared_data_.getChild2Pid();

            bool child1_running = (child1_pid != 0) && shared_data_.checkProcessAlive(child1_pid);
            bool child2_running = (child2_pid != 0) && shared_data_.checkProcessAlive(child2_pid);

            // Запускаем копии, если предыдущие завершились
            if (!child1_running)
            {
                if (launchChildProcess(1))
                {
                    logger_.log("Launched child process 1");
                }
                else
                {
                    logger_.log("Failed to launch child process 1");
                }
            }
            else
            {
                logger_.log("Child process 1 is still running, skipping launch");
            }

            if (!child2_running)
            {
                if (launchChildProcess(2))
                {
                    logger_.log("Launched child process 2");
                }
                else
                {
                    logger_.log("Failed to launch child process 2");
                }
            }
            else
            {
                logger_.log("Child process 2 is still running, skipping launch");
            }
        }
    }

    bool Application::launchChildProcess(int child_type)
    {
#if defined(_WIN32)
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // Создаем командную строку для дочернего процесса
        std::string command = "LAB.exe --child " + std::to_string(child_type);

        if (!CreateProcess(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            return false;
        }

        // Сохраняем PID дочернего процесса
        if (child_type == 1)
        {
            shared_data_.setChild1Pid(pi.dwProcessId);
        }
        else
        {
            shared_data_.setChild2Pid(pi.dwProcessId);
        }

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
#else
        pid_t pid = fork();
        if (pid == 0)
        {
            // Дочерний процесс
            execl("./LAB", "./LAB", "--child", std::to_string(child_type).c_str(), NULL);
            exit(1); // Если execl failed
        }
        else if (pid > 0)
        {
            // Родительский процесс - сохраняем PID
            if (child_type == 1)
            {
                shared_data_.setChild1Pid(pid);
            }
            else
            {
                shared_data_.setChild2Pid(pid);
            }
            return true;
        }
        else
        {
            return false;
        }
#endif
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