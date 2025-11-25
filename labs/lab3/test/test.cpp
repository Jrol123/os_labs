#include "application.hpp"
#include "logger.hpp"
#include "shared_data.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

// Функция для выполнения задачи копии 1
void runChild1()
{
    cplib::Logger logger("process.log");
    cplib::SharedDataManager shared_data("global_counter");

    logger.log("CHILD1 started");
    int current_value = shared_data.getCounter();
    shared_data.setCounter(current_value + 10);
    logger.log("CHILD1 finished");
}

// Функция для выполнения задачи копии 2
void runChild2()
{
    cplib::Logger logger("process.log");
    cplib::SharedDataManager shared_data("global_counter");

    logger.log("CHILD2 started");
    int current_value = shared_data.getCounter();
    shared_data.setCounter(current_value * 2);

    std::this_thread::sleep_for(2000ms);

    current_value = shared_data.getCounter();
    shared_data.setCounter(current_value / 2);
    logger.log("CHILD2 finished");
}

int main(int argc, char **argv)
{
    // Проверяем аргументы командной строки
    if (argc > 1 && std::string(argv[1]) == "--child")
    {
        if (argc > 2)
        {
            int child_type = std::atoi(argv[2]);
            if (child_type == 1)
            {
                runChild1();
            }
            else if (child_type == 2)
            {
                runChild2();
            }
        }
        return 0;
    }

    cplib::Application app("global_counter", "process.log");
    app.run();
    return 0;
}