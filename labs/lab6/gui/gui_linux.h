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
    static void refreshCallback(Widget w, void *client_data, void *call_data);
    static void timerCallback(void *client_data, void *id);

    TemperatureMonitor *monitor_ = nullptr;

    // Widgets
    Widget toplevel_ = nullptr;
    Widget current_temp_value_ = nullptr;
    Widget hourly_avg_value_ = nullptr;
    Widget daily_avg_value_ = nullptr;
    Widget refresh_button_ = nullptr;
    Widget measurement_text_ = nullptr;

    XtAppContext app_context_ = nullptr;

    // Data
    std::string current_temp_str_;
    std::string hourly_avg_str_;
    std::string daily_avg_str_;
};

#endif