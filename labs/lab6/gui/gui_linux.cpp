#include "gui_linux.h"
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/AsciiText.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <mutex>

TemperatureGUILinux &TemperatureGUILinux::getInstance()
{
    static TemperatureGUILinux instance;
    return instance;
}

bool TemperatureGUILinux::initialize(int argc, char **argv)
{
    std::cout << "Initializing Linux GUI..." << std::endl;

    // Initialize Xt toolkit
    XtAppContext app_context;
    toplevel_ = XtVaAppInitialize(
        &app_context,
        "TemperatureMonitor",
        NULL, 0,
        &argc, argv,
        NULL,
        NULL);

    if (!toplevel_)
    {
        std::cerr << "Failed to initialize Xt app" << std::endl;
        return false;
    }

    createControls();

    // Initial display update
    XtAppAddTimeOut(app_context, 100, [](XtPointer client_data, XtIntervalId *id)
                    {
        TemperatureGUILinux::getInstance().updateTemperatureData();
        return False; }, NULL);

    std::cout << "Linux GUI initialized successfully" << std::endl;
    return true;
}

int TemperatureGUILinux::run()
{
    if (!toplevel_)
    {
        std::cerr << "GUI not initialized" << std::endl;
        return 1;
    }

    XtRealizeWidget(static_cast<Widget>(toplevel_));
    XtAppMainLoop(XtWidgetToApplicationContext(static_cast<Widget>(toplevel_)));
    return 0;
}

void TemperatureGUILinux::createControls()
{
    // Create main form with better sizing
    Widget form = XtVaCreateManagedWidget("form", formWidgetClass, static_cast<Widget>(toplevel_),
                                          XtNwidth, 600,
                                          XtNheight, 500,
                                          NULL);

    // Create a box for better layout
    Widget box = XtVaCreateManagedWidget("box", boxWidgetClass, form,
                                         XtNfromHoriz, NULL,
                                         XtNfromVert, NULL,
                                         XtNwidth, 580,
                                         XtNheight, 480,
                                         NULL);

    // Current temperature
    Widget current_temp_label = XtVaCreateManagedWidget("Current Temperature:", labelWidgetClass, box,
                                                        XtNfromHoriz, NULL,
                                                        XtNfromVert, NULL,
                                                        NULL);

    current_temp_value_ = XtVaCreateManagedWidget("Loading...", labelWidgetClass, box,
                                                  XtNfromHoriz, current_temp_label,
                                                  XtNfromVert, NULL,
                                                  NULL);

    // Hourly average
    Widget hourly_avg_label = XtVaCreateManagedWidget("Hourly Average:", labelWidgetClass, box,
                                                      XtNfromHoriz, NULL,
                                                      XtNfromVert, current_temp_label,
                                                      NULL);

    hourly_avg_value_ = XtVaCreateManagedWidget("Loading...", labelWidgetClass, box,
                                                XtNfromHoriz, hourly_avg_label,
                                                XtNfromVert, current_temp_value_,
                                                NULL);

    // Daily average
    Widget daily_avg_label = XtVaCreateManagedWidget("Daily Average:", labelWidgetClass, box,
                                                     XtNfromHoriz, NULL,
                                                     XtNfromVert, hourly_avg_label,
                                                     NULL);

    daily_avg_value_ = XtVaCreateManagedWidget("Loading...", labelWidgetClass, box,
                                               XtNfromHoriz, daily_avg_label,
                                               XtNfromVert, hourly_avg_value_,
                                               NULL);

    // Refresh button
    refresh_button_ = XtVaCreateManagedWidget("Refresh", commandWidgetClass, box,
                                              XtNfromHoriz, NULL,
                                              XtNfromVert, daily_avg_label,
                                              NULL);

    XtAddCallback(static_cast<Widget>(refresh_button_), XtNcallback, [](Widget w, void *client_data, void *call_data)
                  { TemperatureGUILinux::getInstance().updateTemperatureData(); }, NULL);

    // Measurement list
    Widget measurements_label = XtVaCreateManagedWidget("Recent Measurements:", labelWidgetClass, box,
                                                        XtNfromHoriz, NULL,
                                                        XtNfromVert, static_cast<Widget>(refresh_button_),
                                                        NULL);

    measurement_text_ = XtVaCreateManagedWidget("measurement_text", asciiTextWidgetClass, box,
                                                XtNfromHoriz, NULL,
                                                XtNfromVert, measurements_label,
                                                XtNeditType, XawtextRead,
                                                XtNscrollVertical, XawtextScrollAlways,
                                                XtNlength, 4096,
                                                XtNwidth, 550,
                                                XtNheight, 300,
                                                XtNstring, "No measurement data available yet...",
                                                NULL);

    XtManageChild(box);
    XtManageChild(form);
}

void TemperatureGUILinux::refreshCallback(void *user_data)
{
    TemperatureGUILinux *gui = static_cast<TemperatureGUILinux *>(user_data);
    gui->updateTemperatureData();
}

void TemperatureGUILinux::updateTemperatureData()
{
    static std::mutex update_mutex;
    std::lock_guard<std::mutex> lock(update_mutex);

    if (!monitor_)
    {
        monitor_ = &TemperatureMonitor::getInstance();
        if (!monitor_)
        {
            std::cerr << "Monitor not available" << std::endl;
            return;
        }
    }
    updateDisplay();
}

void TemperatureGUILinux::updateDisplay()
{
    if (!monitor_ || !toplevel_)
    {
        std::cerr << "Cannot update display - monitor or toplevel not set" << std::endl;
        return;
    }

    try
    {
        double currentTemp = monitor_->getCurrentTemperature();
        double hourlyAvg = monitor_->getHourlyAverage();
        double dailyAvg = monitor_->getDailyAverage();

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2);

        // Update current temperature
        ss << currentTemp << " °C";
        current_temp_str_ = ss.str();
        XtVaSetValues(static_cast<Widget>(current_temp_value_), XtNlabel, current_temp_str_.c_str(), NULL);
        ss.str("");

        // Update hourly average
        ss << hourlyAvg << " °C";
        hourly_avg_str_ = ss.str();
        XtVaSetValues(static_cast<Widget>(hourly_avg_value_), XtNlabel, hourly_avg_str_.c_str(), NULL);
        ss.str("");

        // Update daily average
        ss << dailyAvg << " °C";
        daily_avg_str_ = ss.str();
        XtVaSetValues(static_cast<Widget>(daily_avg_value_), XtNlabel, daily_avg_str_.c_str(), NULL);
        ss.str("");

        // Update measurement list
        auto recentMeasurements = monitor_->getRecentMeasurements(15);
        std::string text_str = "Time                - Temperature\n";
        text_str += "------------------------------------\n";

        for (const auto &measurement : recentMeasurements)
        {
            std::string timeStr = common::timeToString(measurement.first);
            ss << std::fixed << std::setprecision(2) << measurement.second;
            text_str += timeStr + " - " + ss.str() + " °C\n";
            ss.str("");
        }

        if (recentMeasurements.empty())
        {
            text_str += "No measurements available";
        }

        XtVaSetValues(static_cast<Widget>(measurement_text_), XtNstring, text_str.c_str(), NULL);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error updating display: " << e.what() << std::endl;
        std::string error_msg = "Error: " + std::string(e.what());
        XtVaSetValues(static_cast<Widget>(measurement_text_), XtNstring, error_msg.c_str(), NULL);
    }
}