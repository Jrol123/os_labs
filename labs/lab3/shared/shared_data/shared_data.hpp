#pragma once

#include "../shared_memory.hpp"
#include <string>
#include <chrono>

namespace cplib
{

    struct SharedData
    {
        SharedData() : counter(0), master_pid(0) {}
        int counter;
        int master_pid;                      // PID процесса-мастера
        long long master_timestamp;          // Время когда процесс стал мастером
        long long connection_timestamps[10]; // Времена подключения процессов (упрощенно)
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
        void becomeMaster();
        int getCounter();
        void setCounter(int value);
        void incrementCounter();
        void registerConnection();

        void Lock();
        void Unlock();

    private:
        long long getCurrentTimestamp();
        SharedMem<SharedData> shared_mem_;
    };

} // namespace cplib