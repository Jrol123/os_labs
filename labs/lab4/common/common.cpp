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
        std::cout << "DEBUG: Attempting to delete file: " << path << std::endl;

#ifdef _WIN32
        // На Windows сначала закрываем файл, если он открыт
        HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
        }

        BOOL result = DeleteFileA(path.c_str());
        if (!result)
        {
            DWORD error = GetLastError();
            std::cout << "DEBUG: DeleteFile failed, error code: " << error << std::endl;

            // Попробуем альтернативный метод
            result = std::remove(path.c_str()) == 0;
            if (!result)
            {
                std::cout << "DEBUG: std::remove also failed" << std::endl;
                return false;
            }
        }
        std::cout << "DEBUG: File deleted successfully" << std::endl;
        return true;
#else
        // На UNIX системах
        int result = std::remove(path.c_str());
        if (result != 0)
        {
            std::cout << "DEBUG: std::remove failed, error: " << strerr(errno) << std::endl;
            return false;
        }
        std::cout << "DEBUG: File deleted successfully" << std::endl;
        return true;
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
        // Примеры имен файлов:
        // raw_temperature_20240115_143025.txt
        // hourly_average_20240115.txt
        // daily_average_202401.txt

        try
        {
            std::string dateTimeStr;

            if (filename.find("raw_temperature_") == 0)
            {
                // Формат: raw_temperature_YYYYMMDD_HHMMSS.txt
                size_t start = 16; // длина "raw_temperature_"
                size_t end = filename.find(".txt");
                if (end == std::string::npos || end <= start)
                {
                    std::cout << "DEBUG: Cannot parse raw filename: " << filename << std::endl;
                    return common::currentTime();
                }
                dateTimeStr = filename.substr(start, end - start);

                std::tm tm = {};
                std::istringstream ss(dateTimeStr);
                ss >> std::get_time(&tm, "%Y%m%d_%H%M%S");

                if (ss.fail())
                {
                    std::cout << "DEBUG: Failed to parse raw datetime: " << dateTimeStr << std::endl;
                    return common::currentTime();
                }

                return std::chrono::system_clock::from_time_t(std::mktime(&tm));
            }
            else if (filename.find("hourly_average_") == 0)
            {
                // Формат: hourly_average_YYYYMMDD.txt
                size_t start = 15; // длина "hourly_average_"
                size_t end = filename.find(".txt");
                if (end == std::string::npos || end <= start)
                {
                    std::cout << "DEBUG: Cannot parse hourly filename: " << filename << std::endl;
                    return common::currentTime();
                }
                dateTimeStr = filename.substr(start, end - start);

                std::tm tm = {};
                std::istringstream ss(dateTimeStr);
                ss >> std::get_time(&tm, "%Y%m%d");
                tm.tm_hour = 12; // Устанавливаем полдень для корректного сравнения
                tm.tm_min = 0;
                tm.tm_sec = 0;

                if (ss.fail())
                {
                    std::cout << "DEBUG: Failed to parse hourly date: " << dateTimeStr << std::endl;
                    return common::currentTime();
                }

                return std::chrono::system_clock::from_time_t(std::mktime(&tm));
            }
            else if (filename.find("daily_average_") == 0)
            {
                // Формат: daily_average_YYYYMM.txt
                size_t start = 14; // длина "daily_average_"
                size_t end = filename.find(".txt");
                if (end == std::string::npos || end <= start)
                {
                    std::cout << "DEBUG: Cannot parse daily filename: " << filename << std::endl;
                    return common::currentTime();
                }
                dateTimeStr = filename.substr(start, end - start);

                std::tm tm = {};
                std::istringstream ss(dateTimeStr);
                ss >> std::get_time(&tm, "%Y%m");
                tm.tm_mday = 15; // Устанавливаем середину месяца для корректного сравнения
                tm.tm_hour = 12;
                tm.tm_min = 0;
                tm.tm_sec = 0;

                if (ss.fail())
                {
                    std::cout << "DEBUG: Failed to parse daily date: " << dateTimeStr << std::endl;
                    return common::currentTime();
                }

                return std::chrono::system_clock::from_time_t(std::mktime(&tm));
            }
            else
            {
                std::cout << "DEBUG: Unknown file type: " << filename << std::endl;
                return common::currentTime();
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "DEBUG: Exception in parseTimeFromFileName: " << e.what() << std::endl;
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