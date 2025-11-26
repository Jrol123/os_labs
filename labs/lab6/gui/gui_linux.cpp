#include "gui_linux.h"
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <sstream>
#include <iomanip>
#include <iostream>

TemperatureGUILinux &TemperatureGUILinux::getInstance()
{
    static TemperatureGUILinux instance;
    return instance;
}

bool TemperatureGUILinux::initialize(int argc, char **argv)
{
    // Initialize Xt toolkit
    toplevel_ = XtVaAppInitialize(
        nullptr,              // app_context
        "TemperatureMonitor", // application_class
        nullptr, 0,           // options
        &argc, argv,          // command line
        nullptr,              // fallback_resources
        nullptr               // args
    );

    if (!toplevel_)
    {
        std::cerr << "Failed to initialize Xt app" << std::endl;
        return false;
    }

    createControls();
    updateDisplay();

    return true;
}

int TemperatureGUILinux::run()
{
    XtRealizeWidget(static_cast<Widget>(toplevel_));
    XtAppMainLoop(XtWidgetToApplicationContext(static_cast<Widget>(toplevel_)));
    return 0;
}

void TemperatureGUILinux::createControls()
{
    // Create main form
    Widget form = XtVaCreateWidget("form", formWidgetClass, static_cast<Widget>(toplevel_), nullptr);

    // Create temperature labels and values
    Widget current_temp_label = XtVaCreateManagedWidget("Current Temperature:", labelWidgetClass, form,
                                                        XtNfromHoriz, nullptr,
                                                        XtNfromVert, nullptr,
                                                        nullptr);

    current_temp_value_ = XtVaCreateManagedWidget("Loading...", labelWidgetClass, form,
                                                  XtNfromHoriz, current_temp_label,
                                                  XtNfromVert, nullptr,
                                                  nullptr);

    Widget hourly_avg_label = XtVaCreateManagedWidget("Hourly Average:", labelWidgetClass, form,
                                                      XtNfromHoriz, nullptr,
                                                      XtNfromVert, current_temp_label,
                                                      nullptr);

    hourly_avg_value_ = XtVaCreateManagedWidget("Loading...", labelWidgetClass, form,
                                                XtNfromHoriz, hourly_avg_label,
                                                XtNfromVert, current_temp_value_,
                                                nullptr);

    Widget daily_avg_label = XtVaCreateManagedWidget("Daily Average:", labelWidgetClass, form,
                                                     XtNfromHoriz, nullptr,
                                                     XtNfromVert, hourly_avg_label,
                                                     nullptr);

    daily_avg_value_ = XtVaCreateManagedWidget("Loading...", labelWidgetClass, form,
                                               XtNfromHoriz, daily_avg_label,
                                               XtNfromVert, hourly_avg_value_,
                                               nullptr);

    // Refresh button
    refresh_button_ = XtVaCreateManagedWidget("Refresh", commandWidgetClass, form,
                                              XtNfromHoriz, nullptr,
                                              XtNfromVert, daily_avg_label,
                                              nullptr);

    XtAddCallback(static_cast<Widget>(refresh_button_), XtNcallback, [](Widget w, void *client_data, void *call_data)
                  { static_cast<TemperatureGUILinux *>(client_data)->refreshCallback(client_data); }, this);

    // Measurement list as text widget
    measurement_text_ = XtVaCreateManagedWidget("measurement_text", textWidgetClass, form,
                                                XtNfromHoriz, nullptr,
                                                XtNfromVert, static_cast<Widget>(refresh_button_),
                                                XtNeditType, XawtextRead,
                                                XtNlength, 1024,
                                                XtNstring, "No data",
                                                nullptr);

    XtManageChild(form);
}

void TemperatureGUILinux::refreshCallback(void *user_data)
{
    TemperatureGUILinux *gui = static_cast<TemperatureGUILinux *>(user_data);
    gui->updateTemperatureData();
}

void TemperatureGUILinux::updateTemperatureData()
{
    if (!monitor_)
    {
        monitor_ = &TemperatureMonitor::getInstance();
        if (!monitor_)
            return;
    }
    updateDisplay();
}

void TemperatureGUILinux::updateDisplay()
{
    if (!monitor_)
        return;

    double currentTemp = monitor_->getCurrentTemperature();
    double hourlyAvg = monitor_->getHourlyAverage();
    double dailyAvg = monitor_->getDailyAverage();

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);

    // Update current temperature
    ss << currentTemp << " °C";
    current_temp_str_ = ss.str();
    XtVaSetValues(static_cast<Widget>(current_temp_value_), XtNlabel, current_temp_str_.c_str(), nullptr);
    ss.str("");

    // Update hourly average
    ss << hourlyAvg << " °C";
    hourly_avg_str_ = ss.str();
    XtVaSetValues(static_cast<Widget>(hourly_avg_value_), XtNlabel, hourly_avg_str_.c_str(), nullptr);
    ss.str("");

    // Update daily average
    ss << dailyAvg << " °C";
    daily_avg_str_ = ss.str();
    XtVaSetValues(static_cast<Widget>(daily_avg_value_), XtNlabel, daily_avg_str_.c_str(), nullptr);
    ss.str("");

    // Update measurement list
    auto recentMeasurements = monitor_->getRecentMeasurements(20);
    std::string text_str;
    for (const auto &measurement : recentMeasurements)
    {
        std::string timeStr = common::timeToString(measurement.first);
        ss << std::fixed << std::setprecision(2) << measurement.second << " °C";
        text_str += timeStr + " - " + ss.str() + "\n";
        ss.str("");
    }
    XtVaSetValues(static_cast<Widget>(measurement_text_), XtNstring, text_str.c_str(), nullptr);
}