#include "shared_counter.hpp"
#include <string>

namespace cplib
{

    SharedCounter::SharedCounter(const std::string &name)
        : counterMem(name.c_str())
    {
    }

    int SharedCounter::getValue()
    {
        counterMem.Lock();
        int value = counterMem.Data()->value;
        counterMem.Unlock();
        return value;
    }

    void SharedCounter::setValue(int value)
    {
        counterMem.Lock();
        counterMem.Data()->value = value;
        counterMem.Unlock();
    }

    void SharedCounter::increment()
    {
        counterMem.Lock();
        counterMem.Data()->value++;
        counterMem.Unlock();
    }

}