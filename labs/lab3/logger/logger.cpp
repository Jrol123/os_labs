#include "logger.hpp"
#include <sstream>

namespace cplib
{

    Logger::Logger(const std::string &filename)
    {
#if defined(_WIN32)
        processId = GetCurrentProcessId();
#else
        processId = getpid();
#endif

        logFile.open(filename, std::ios::app);
    }

    Logger::~Logger()
    {
        if (logFile.is_open())
        {
            logFile.close();
        }
    }

    void Logger::log(const std::string &message)
    {
        if (logFile.is_open())
        {
            logFile << "[" << getCurrentTime(true) << "] PID " << processId << ": " << message << std::endl;
        }
    }

    void Logger::logStartup()
    {
        log("Process started at " + getCurrentTime());
    }

    void Logger::logCounter(int counter)
    {
        std::stringstream ss;
        ss << "Counter value: " << counter;
        log(ss.str());
    }

    std::string Logger::getCurrentTime(bool withMilliseconds)
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (withMilliseconds)
        {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch()) %
                      1000;
            ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        }

        return ss.str();
    }

} // namespace cplib