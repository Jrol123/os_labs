#pragma once

#include "../shared_memory.hpp"
#include <string>
#include <chrono>

namespace cplib
{

    struct SharedData
    {
        SharedData() : counter(0), master_pid(0), child1_pid(0), child2_pid(0), master_timestamp(0), connection_count(0) {}
        int counter;
        int master_pid;                      // PID процесса-мастера
        int child1_pid;                      // PID первой копии
        int child2_pid;                      // PID второй копии
        long long master_timestamp;          // Время когда процесс стал мастером
        long long connection_timestamps[10]; // Времена подключения процессов

        int connection_count;
    };

    class SharedDataManager
    {
    public:
        SharedDataManager(const std::string &name);
        ~SharedDataManager() = default;

        bool isValid() { return shared_mem_.IsValid(); }
        bool isMaster();
        bool checkMasterAlive();
        bool checkProcessAlive(int pid);
        void becomeMaster();
        int getCounter();
        void setCounter(int value);
        void incrementCounter();
        void registerConnection();

        int getChild1Pid();
        void setChild1Pid(int pid);
        int getChild2Pid();
        void setChild2Pid(int pid);

        void Lock();
        void Unlock();

    private:
        long long getCurrentTimestamp();
        SharedMem<SharedData> shared_mem_;
    };

}