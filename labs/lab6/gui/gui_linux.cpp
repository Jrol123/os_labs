#include "gui_linux.h"
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <vector>

TemperatureGUILinux &TemperatureGUILinux::getInstance()
{
    static TemperatureGUILinux instance;
    return instance;
}

bool TemperatureGUILinux::initialize(int argc, char **argv)
{
    std::cout << "Initializing Linux GUI..." << std::endl;

    // Initialize Xt toolkit
    toplevel_ = XtVaAppInitialize(
        &app_context_,
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

    XtRealizeWidget(toplevel_);

    // Setup timer for periodic updates (2000ms)
    XtAppAddTimeOut(app_context_, 2000, (XtTimerCallbackProc)timerCallback, this);

    XtAppMainLoop(app_context_);
    return 0;
}

void TemperatureGUILinux::timerCallback(void *client_data, void *id)
{
    TemperatureGUILinux *gui = static_cast<TemperatureGUILinux *>(client_data);
    if (gui)
    {
        gui->updateTemperatureData();
        // Reschedule timer
        XtAppAddTimeOut(gui->app_context_, 2000, (XtTimerCallbackProc)timerCallback, (XtPointer)gui);
    }
}

void TemperatureGUILinux::createControls()
{
    // Create main form
    main_form_ = XtVaCreateManagedWidget("main_form", formWidgetClass, toplevel_,
                                         XtNwidth, 800,
                                         XtNheight, 600,
                                         NULL);

    // Current temperature
    Widget current_temp_label = XtVaCreateManagedWidget("Current Temperature:", labelWidgetClass, main_form_,
                                                        XtNfromHoriz, NULL,
                                                        XtNfromVert, NULL,
                                                        XtNx, 20,
                                                        XtNy, 20,
                                                        XtNwidth, 200,
                                                        XtNheight, 25,
                                                        NULL);

    current_temp_value_ = XtVaCreateManagedWidget("Loading...", labelWidgetClass, main_form_,
                                                  XtNfromHoriz, current_temp_label,
                                                  XtNfromVert, NULL,
                                                  XtNx, 220,
                                                  XtNy, 20,
                                                  XtNwidth, 100,
                                                  XtNheight, 25,
                                                  NULL);

    // Hourly average
    Widget hourly_avg_label = XtVaCreateManagedWidget("Hourly Average:", labelWidgetClass, main_form_,
                                                      XtNfromHoriz, NULL,
                                                      XtNfromVert, current_temp_label,
                                                      XtNx, 20,
                                                      XtNy, 50,
                                                      XtNwidth, 200,
                                                      XtNheight, 25,
                                                      NULL);

    hourly_avg_value_ = XtVaCreateManagedWidget("Loading...", labelWidgetClass, main_form_,
                                                XtNfromHoriz, hourly_avg_label,
                                                XtNfromVert, current_temp_value_,
                                                XtNx, 220,
                                                XtNy, 50,
                                                XtNwidth, 100,
                                                XtNheight, 25,
                                                NULL);

    // Daily average
    Widget daily_avg_label = XtVaCreateManagedWidget("Daily Average:", labelWidgetClass, main_form_,
                                                     XtNfromHoriz, NULL,
                                                     XtNfromVert, hourly_avg_label,
                                                     XtNx, 20,
                                                     XtNy, 80,
                                                     XtNwidth, 200,
                                                     XtNheight, 25,
                                                     NULL);

    daily_avg_value_ = XtVaCreateManagedWidget("Loading...", labelWidgetClass, main_form_,
                                               XtNfromHoriz, daily_avg_label,
                                               XtNfromVert, hourly_avg_value_,
                                               XtNx, 220,
                                               XtNy, 80,
                                               XtNwidth, 100,
                                               XtNheight, 25,
                                               NULL);

    // Refresh button
    refresh_button_ = XtVaCreateManagedWidget("Refresh", commandWidgetClass, main_form_,
                                              XtNfromHoriz, NULL,
                                              XtNfromVert, daily_avg_label,
                                              XtNx, 350,
                                              XtNy, 20,
                                              XtNwidth, 100,
                                              XtNheight, 30,
                                              NULL);

    XtAddCallback(refresh_button_, XtNcallback, refreshCallback, this);

    // Measurement list label
    Widget list_label = XtVaCreateManagedWidget("Recent Measurements:", labelWidgetClass, main_form_,
                                                XtNfromHoriz, NULL,
                                                XtNfromVert, refresh_button_,
                                                XtNx, 20,
                                                XtNy, 120,
                                                XtNwidth, 200,
                                                XtNheight, 25,
                                                NULL);

    // Create a scrollable text widget for measurements
    measurement_list_ = XtVaCreateManagedWidget("measurement_list", asciiTextWidgetClass, main_form_,
                                                XtNfromHoriz, NULL,
                                                XtNfromVert, list_label,
                                                XtNx, 20,
                                                XtNy, 150,
                                                XtNwidth, 740,
                                                XtNheight, 400,
                                                XtNeditType, XawtextRead,
                                                XtNscrollVertical, XawtextScrollAlways,
                                                XtNlength, 65536,
                                                XtNstring, "Loading measurements...",
                                                NULL);

    XtManageChild(main_form_);
}

void TemperatureGUILinux::refreshCallback(Widget w, void *client_data, void *call_data)
{
    TemperatureGUILinux *gui = static_cast<TemperatureGUILinux *>(client_data);
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
        XtVaSetValues(current_temp_value_, XtNlabel, current_temp_str_.c_str(), NULL);
        ss.str("");

        // Update hourly average
        ss << hourlyAvg << " °C";
        hourly_avg_str_ = ss.str();
        XtVaSetValues(hourly_avg_value_, XtNlabel, hourly_avg_str_.c_str(), NULL);
        ss.str("");

        // Update daily average
        ss << dailyAvg << " °C";
        daily_avg_str_ = ss.str();
        XtVaSetValues(daily_avg_value_, XtNlabel, daily_avg_str_.c_str(), NULL);
        ss.str("");

        // Update measurement list as formatted text
        auto recentMeasurements = monitor_->getRecentMeasurements(20);
        std::string text_str = "Time                Temperature  Status\n";
        text_str += "--------------------------------------------------\n";

        for (const auto &measurement : recentMeasurements)
        {
            std::string timeStr = common::timeToString(measurement.first);
            double temp = measurement.second;

            ss << std::fixed << std::setprecision(2) << temp;
            std::string tempStr = ss.str() + " °C";
            ss.str("");

            // Determine status with visual indicators
            std::string statusStr;
            if (temp >= 18 && temp <= 28)
            {
                statusStr = "● Normal";
            }
            else if ((temp >= 15 && temp < 18) || (temp > 28 && temp <= 32))
            {
                statusStr = "● Warning";
            }
            else
            {
                statusStr = "● Critical";
            }

            // Format with fixed column widths
            std::string line = timeStr + std::string(20 - std::min(20, (int)timeStr.length()), ' ') +
                               tempStr + std::string(15 - std::min(15, (int)tempStr.length()), ' ') +
                               statusStr + "\n";

            text_str += line;
        }

        if (recentMeasurements.empty())
        {
            text_str += "No measurements available\n";
        }

        // Update the text widget
        XtVaSetValues(measurement_list_, XtNstring, text_str.c_str(), NULL);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error updating display: " << e.what() << std::endl;
        std::string error_msg = "Error loading data:\n" + std::string(e.what());
        XtVaSetValues(measurement_list_, XtNstring, error_msg.c_str(), NULL);
    }
}