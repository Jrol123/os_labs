#pragma once
#include "../shared_memory.hpp"
#include <string>

namespace cplib
{

    struct CounterData
    {
        CounterData() : value(0) {}
        int value;
    };

    class SharedCounter
    {
    public:
        SharedCounter(const std::string &name);
        ~SharedCounter() = default;

        bool isValid() const
        {
            return const_cast<SharedMem<CounterData> &>(counterMem).IsValid();
        }
        int getValue();
        void setValue(int value);
        void increment();

    private:
        SharedMem<CounterData> counterMem;
    };

} // namespace cplib