#ifndef GUI_H
#define GUI_H

#include "temperature_monitor.h"
#include <windows.h>
#include <string>
#include <vector>

class TemperatureGUI
{
public:
    static TemperatureGUI &getInstance();
    bool initialize(HINSTANCE hInstance);
    int run();

    void updateTemperatureData();
    void setMonitor(TemperatureMonitor *monitor) { monitor_ = monitor; }

    std::wstring stringToWString(const std::string &str);
    std::string wstringToString(const std::wstring& wstr);

private:
    TemperatureGUI() = default;
    ~TemperatureGUI() = default;
    // Запрет копирования
    TemperatureGUI(const TemperatureGUI &) = delete;
    TemperatureGUI &operator=(const TemperatureGUI &) = delete;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void createControls(HWND hwnd);
    void updateDisplay();

    HWND hwnd_ = nullptr;
    HINSTANCE hInstance_ = nullptr;

    // Элементы управления
    HWND currentTempLabel_ = nullptr;
    HWND currentTempValue_ = nullptr;
    HWND hourlyAvgLabel_ = nullptr;
    HWND hourlyAvgValue_ = nullptr;
    HWND dailyAvgLabel_ = nullptr;
    HWND dailyAvgValue_ = nullptr;
    HWND refreshButton_ = nullptr;
    HWND measurementList_ = nullptr;

    TemperatureMonitor *monitor_ = nullptr;

    // Идентификаторы элементов
    enum ControlIDs
    {
        ID_CURRENT_TEMP_LABEL = 1001,
        ID_CURRENT_TEMP_VALUE,
        ID_HOURLY_AVG_LABEL,
        ID_HOURLY_AVG_VALUE,
        ID_DAILY_AVG_LABEL,
        ID_DAILY_AVG_VALUE,
        ID_REFRESH_BUTTON,
        ID_MEASUREMENT_LIST
    };
};

#endif