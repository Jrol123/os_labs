#include "gui_linux.h"
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Scrollbar.h>
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

    // Create a container for the list with scrollbar
    list_container_ = XtVaCreateManagedWidget("list_container", boxWidgetClass, main_form_,
                                              XtNfromHoriz, NULL,
                                              XtNfromVert, list_label,
                                              XtNx, 20,
                                              XtNy, 150,
                                              XtNwidth, 740,
                                              XtNheight, 400,
                                              NULL);

    // Create list widget
    measurement_list_ = XtVaCreateManagedWidget("measurement_list", listWidgetClass, list_container_,
                                                XtNwidth, 720,
                                                XtNheight, 380,
                                                XtNlist, NULL, // Will be set dynamically
                                                XtNnumberStrings, 0,
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

        // Update measurement list
        auto recentMeasurements = monitor_->getRecentMeasurements(20);
        list_items_.clear();

        // Add header
        list_items_.push_back("Time                Temperature  Status");

        for (const auto &measurement : recentMeasurements)
        {
            std::string timeStr = common::timeToString(measurement.first);
            double temp = measurement.second;

            ss << std::fixed << std::setprecision(2) << temp;
            std::string tempStr = ss.str() + " °C";
            ss.str("");

            // Determine status with color indicators
            std::string statusStr;
            if (temp >= 18 && temp <= 28)
            {
                statusStr = "● Normal"; // Green bullet
            }
            else if ((temp >= 15 && temp < 18) || (temp > 28 && temp <= 32))
            {
                statusStr = "● Warning"; // Yellow bullet
            }
            else
            {
                statusStr = "● Critical"; // Red bullet
            }

            // Format the line with fixed column widths
            std::string line = timeStr + std::string(20 - timeStr.length(), ' ') +
                               tempStr + std::string(12 - tempStr.length(), ' ') +
                               statusStr;

            list_items_.push_back(line);
        }

        if (recentMeasurements.empty())
        {
            list_items_.push_back("No measurements available");
        }

        // Convert vector to Xt StringList
        XawListReturnStruct **string_list = (XawListReturnStruct **)XtMalloc(sizeof(XawListReturnStruct *) * (list_items_.size() + 1));

        for (size_t i = 0; i < list_items_.size(); ++i)
        {
            string_list[i] = (XawListReturnStruct *)XtMalloc(sizeof(XawListReturnStruct));
            string_list[i]->string = XtNewString(list_items_[i].c_str());
            string_list[i]->text_proc = NULL;
        }
        string_list[list_items_.size()] = NULL;

        // Update the list
        XtVaSetValues(measurement_list_,
                      XtNlist, string_list,
                      XtNnumberStrings, list_items_.size(),
                      NULL);

        // Free the allocated memory
        for (size_t i = 0; i < list_items_.size(); ++i)
        {
            XtFree(string_list[i]->string);
            XtFree((char *)string_list[i]);
        }
        XtFree((char *)string_list);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error updating display: " << e.what() << std::endl;

        // Show error in list
        list_items_.clear();
        list_items_.push_back("Error loading data:");
        list_items_.push_back(e.what());

        XawListReturnStruct **string_list = (XawListReturnStruct **)XtMalloc(sizeof(XawListReturnStruct *) * 3);
        string_list[0] = (XawListReturnStruct *)XtMalloc(sizeof(XawListReturnStruct));
        string_list[0]->string = XtNewString(list_items_[0].c_str());
        string_list[0]->text_proc = NULL;

        string_list[1] = (XawListReturnStruct *)XtMalloc(sizeof(XawListReturnStruct));
        string_list[1]->string = XtNewString(list_items_[1].c_str());
        string_list[1]->text_proc = NULL;

        string_list[2] = NULL;

        XtVaSetValues(measurement_list_,
                      XtNlist, string_list,
                      XtNnumberStrings, 2,
                      NULL);

        XtFree(string_list[0]->string);
        XtFree((char *)string_list[0]);
        XtFree(string_list[1]->string);
        XtFree((char *)string_list[1]);
        XtFree((char *)string_list);
    }
}