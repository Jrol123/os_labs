#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <chrono>
#include <ctime>

// Кроссплатформенные определения
#ifdef _WIN32
    #include <windows.h>
    #define PATH_SEPARATOR "\\"
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #define PATH_SEPARATOR "/"
#endif

namespace common {
    
    // Типы для времени
    using TimePoint = std::chrono::system_clock::time_point;
    using Duration = std::chrono::system_clock::duration;
    
    // Получить текущее время
    inline TimePoint currentTime() {
        return std::chrono::system_clock::now();
    }
    
    // Преобразовать время в строку
    std::string timeToString(const TimePoint& time);
    
    // Преобразовать время в строку для имени файла
    std::string timeToFileName(const TimePoint& time);
    
    // Создать директорию если не существует
    bool createDirectory(const std::string& path);
    
    // Получить путь к директории для логов
    std::string getLogDirectory();
    
    // Проверить существование файла
    bool fileExists(const std::string& path);

} // namespace common

#endif