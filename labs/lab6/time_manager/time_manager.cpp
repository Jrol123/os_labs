#include "time_manager.h"

TimeManager &TimeManager::getInstance()
{
    static TimeManager instance;
    return instance;
}

void TimeManager::setTimeConfig(const TimeConfig &config)
{
    config_ = config;

    if (config.mode == TimeMode::CUSTOM_TIME)
    {
        custom_time_start_ = common::currentTime();
        real_time_start_ = common::currentTime();
        custom_time_active_ = true;
    }
    else
    {
        custom_time_active_ = false;
    }
}

common::TimePoint TimeManager::getCurrentTime()
{
    if (!custom_time_active_)
    {
        return common::currentTime();
    }

    auto real_now = common::currentTime();
    auto real_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        real_now - real_time_start_);

    auto custom_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        real_elapsed * config_.time_scale);

    return custom_time_start_ + custom_elapsed;
}

common::TimePoint TimeManager::toCustomTime(const common::TimePoint &real_time)
{
    if (!custom_time_active_)
    {
        return real_time;
    }

    auto real_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        real_time - real_time_start_);

    auto custom_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        real_elapsed * config_.time_scale);

    return custom_time_start_ + custom_elapsed;
}

common::TimePoint TimeManager::toRealTime(const common::TimePoint &custom_time)
{
    if (!custom_time_active_)
    {
        return custom_time;
    }

    auto custom_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        custom_time - custom_time_start_);

    auto real_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        custom_elapsed / config_.time_scale);

    return real_time_start_ + real_elapsed;
}

void TimeManager::resetToRealTime()
{
    custom_time_active_ = false;
    config_.mode = TimeMode::REAL_TIME;
    config_.time_scale = 1.0;
}