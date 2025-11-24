#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace cplib
{

    class Logger
    {
    public:
        Logger(const std::string &filename);
        ~Logger();

        void log(const std::string &message);
        void logStartup();
        void logCounter(int counter);
        std::string getCurrentTime(bool withMilliseconds = false);

    private:
        std::ofstream logFile;
        int processId;
    };

} // namespace cplib