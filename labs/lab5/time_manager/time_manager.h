#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include "common.h"
#include <chrono>
#include <ratio>

class TimeManager
{
public:
    // Режимы работы времени
    enum class TimeMode
    {
        REAL_TIME,  // Реальное время
        CUSTOM_TIME // Кастомное время с ускорением
    };

    // Конфигурация кастомного времени
    struct TimeConfig
    {
        TimeMode mode = TimeMode::REAL_TIME;
        double time_scale = 1.0; // Масштаб времени (1.0 = реальное время)

        // Кастомные длительности для тестирования
        std::chrono::milliseconds custom_hour = std::chrono::hours(1);
        std::chrono::milliseconds custom_day = std::chrono::hours(24);
        std::chrono::milliseconds custom_year = std::chrono::hours(365 * 24);
    };

    static TimeManager &getInstance();

    // Установка конфигурации времени
    void setTimeConfig(const TimeConfig &config);

    // Получение текущего времени (реального или кастомного)
    common::TimePoint getCurrentTime();

    // Преобразование реального времени в кастомное
    common::TimePoint toCustomTime(const common::TimePoint &real_time);

    // Преобразование кастомного времени в реальное
    common::TimePoint toRealTime(const common::TimePoint &custom_time);

    // Получение кастомных интервалов
    std::chrono::milliseconds getCustomHour() const { return config_.custom_hour; }
    std::chrono::milliseconds getCustomDay() const { return config_.custom_day; }
    std::chrono::milliseconds getCustomYear() const { return config_.custom_year; }

    // Сброс к реальному времени
    void resetToRealTime();

private:
    TimeManager() = default;

    TimeConfig config_;
    common::TimePoint custom_time_start_; // Начало кастомного времени
    common::TimePoint real_time_start_;   // Соответствующее реальное время
    bool custom_time_active_ = false;
};

#endif