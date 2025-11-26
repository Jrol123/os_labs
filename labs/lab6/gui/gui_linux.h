#ifndef GUI_LINUX_H
#define GUI_LINUX_H

#include "temperature_monitor.h"
#include <string>
#include <vector>

class TemperatureGUILinux
{
public:
    static TemperatureGUILinux &getInstance();
    bool initialize(int argc, char **argv);
    int run();
    void updateTemperatureData();
    void setMonitor(TemperatureMonitor *monitor) { monitor_ = monitor; }

private:
    TemperatureGUILinux() = default;
    ~TemperatureGUILinux() = default;

    void createControls();
    void updateDisplay();
    static void refreshCallback(void *user_data);

    TemperatureMonitor *monitor_ = nullptr;

    // Widgets
    void *toplevel_ = nullptr;
    void *current_temp_value_ = nullptr;
    void *hourly_avg_value_ = nullptr;
    void *daily_avg_value_ = nullptr;
    void *refresh_button_ = nullptr;
    void *measurement_text_ = nullptr;

    // Data
    std::string current_temp_str_;
    std::string hourly_avg_str_;
    std::string daily_avg_str_;
    std::vector<std::string> measurement_list_data_;
};

#endif