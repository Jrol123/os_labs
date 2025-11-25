#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <chrono>
#include <ctime>
#include <vector>
#include <filesystem>

// Кроссплатформенные определения
#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR "\\"
#else
#include <unistd.h>
#include <sys/stat.h>
#define PATH_SEPARATOR "/"
#endif

// Общие функции
namespace common
{

    // Типы для времени
    using TimePoint = std::chrono::system_clock::time_point;
    using Duration = std::chrono::system_clock::duration;

    // Получить текущее время
    inline TimePoint currentTime()
    {
        return std::chrono::system_clock::now();
    }

    // Преобразовать время в строку
    std::string timeToString(const TimePoint &time);

    // Преобразовать время в строку для имени файла
    std::string timeToFileName(const TimePoint &time);

    // Создать директорию если не существует
    bool createDirectory(const std::string &path);

    // Получить путь к директории для логов
    std::string getLogDirectory();

    // Проверить существование файла
    bool fileExists(const std::string &path);
    // Получить список файлов в папке
    std::vector<std::string> getFilesInDirectory(const std::string &directory);
    // Удалить файл
    bool deleteFile(const std::string &path);
    // Получить дату
    std::string getDateString(const TimePoint &time);
    // Получить часы
    std::string getHourString(const TimePoint &time);
    // Получить время из названия файла
    TimePoint parseTimeFromFileName(const std::string &filename);

    TimePoint getCurrentTime(); // Использует TimeManager
    void setupCustomTime(
                         std::chrono::milliseconds custom_hour = std::chrono::minutes(10),
                         std::chrono::milliseconds custom_day = std::chrono::hours(2),
                         std::chrono::milliseconds custom_year = std::chrono::hours(48));
    void resetToRealTime();

} // namespace common

#endif