#include "common.h"
#include "time_manager.h"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <dirent.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace common
{

    std::string timeToString(const TimePoint &time)
    {
        auto time_t = std::chrono::system_clock::to_time_t(time);
        auto tm = *std::localtime(&time_t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string timeToFileName(const TimePoint &time)
    {
        auto time_t = std::chrono::system_clock::to_time_t(time);
        auto tm = *std::localtime(&time_t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
        return oss.str();
    }

    bool createDirectory(const std::string &path)
    {
#ifdef _WIN32
        return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
        return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
    }

    std::string getLogDirectory()
    {
        return "logs";
    }

    bool fileExists(const std::string &path)
    {
#ifdef _WIN32
        DWORD attrib = GetFileAttributesA(path.c_str());
        return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
        struct stat buffer;
        return stat(path.c_str(), &buffer) == 0;
#endif
    }
    std::vector<std::string> getFilesInDirectory(const std::string &directory)
    {
        std::vector<std::string> files;

#ifdef _WIN32
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA((directory + "\\*").c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    files.push_back(findData.cFileName);
                }
            } while (FindNextFileA(hFind, &findData));
            FindClose(hFind);
        }
#else
        DIR *dir = opendir(directory.c_str());
        if (dir)
        {
            struct dirent *entry;
            while ((entry = readdir(dir)) != nullptr)
            {
                if (entry->d_type == DT_REG)
                { // Regular file
                    files.push_back(entry->d_name);
                }
            }
            closedir(dir);
        }
#endif

        return files;
    }

    bool deleteFile(const std::string &path)
    {
#ifdef _WIN32
        return DeleteFileA(path.c_str()) != 0;
#else
        return std::remove(path.c_str()) == 0;
#endif
    }

    std::string getDateString(const TimePoint &time)
    {
        auto time_t = std::chrono::system_clock::to_time_t(time);
        auto tm = *std::localtime(&time_t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d");
        return oss.str();
    }

    std::string getHourString(const TimePoint &time)
    {
        auto time_t = std::chrono::system_clock::to_time_t(time);
        auto tm = *std::localtime(&time_t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%H");
        return oss.str();
    }

    TimePoint parseTimeFromFileName(const std::string &filename)
    {
        // Пример: temperature_log_20240115_143025.txt
        try
        {
            size_t start = filename.find("_") + 1;
            size_t end = filename.find(".txt");
            if (start == std::string::npos || end == std::string::npos)
            {
                return common::currentTime();
            }

            std::string dateTimeStr = filename.substr(start, end - start);
            std::tm tm = {};
            std::istringstream ss(dateTimeStr);
            ss >> std::get_time(&tm, "%Y%m%d_%H%M%S");

            if (ss.fail())
            {
                return common::currentTime();
            }

            return std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }
        catch (...)
        {
            return common::currentTime();
        }
    }

    TimePoint getCurrentTime()
    {
        return TimeManager::getInstance().getCurrentTime();
    }

    void setupCustomTime(
        std::chrono::milliseconds custom_hour,
        std::chrono::milliseconds custom_day,
        std::chrono::milliseconds custom_year)
    {
        TimeManager::TimeConfig config;
        config.mode = TimeManager::TimeMode::CUSTOM_TIME;
        // config.time_scale = time_scale;
        config.custom_hour = custom_hour;
        config.custom_day = custom_day;
        config.custom_year = custom_year;

        TimeManager::getInstance().setTimeConfig(config);
    }

    void resetToRealTime()
    {
        TimeManager::getInstance().resetToRealTime();
    }

} // namespace common