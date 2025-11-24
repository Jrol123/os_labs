#pragma once

#include "../shared_memory.hpp"
#include <string>

namespace cplib {

struct SharedData {
    SharedData() : counter(0), master_pid(0) {}
    int counter;
    int master_pid;  // PID процесса-мастера
};

class SharedDataManager {
public:
    SharedDataManager(const std::string& name);
    ~SharedDataManager() = default;
    
    bool isValid() { return shared_mem_.IsValid(); }
    bool isMaster();
    int getCounter();
    void setCounter(int value);
    void incrementCounter();
    
private:
    SharedMem<SharedData> shared_mem_;
};

} // namespace cplib