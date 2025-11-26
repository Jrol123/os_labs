#include "gui.h"
#include <sstream>
#include <iomanip>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

TemperatureGUI &TemperatureGUI::getInstance()
{
    static TemperatureGUI instance;
    return instance;
}

bool TemperatureGUI::initialize(HINSTANCE hInstance)
{
    hInstance_ = hInstance;
    // Инициализация common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // Регистрация класса окна
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = (LPSTR) "TemperatureMonitorGUI";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        return false;
    }

    // Создание главного окна
    hwnd_ = CreateWindowEx(
        0,
        (LPSTR) "TemperatureMonitorGUI",
        (LPSTR) "Temperature Monitoring System",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, hInstance, this);

    if (!hwnd_)
    {
        return false;
    }

    // Установка указателя на экземпляр класса в user data окна
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, (LONG_PTR)this);

    createControls(hwnd_);
    updateTemperatureData();

    return true;
}

int TemperatureGUI::run()
{
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    // Таймер для автоматического обновления данных (каждые 5 секунд)
    SetTimer(hwnd_, 1, 5000, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK TemperatureGUI::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TemperatureGUI *pThis = nullptr;
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
        pThis = (TemperatureGUI *)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    }
    else
    {
        pThis = (TemperatureGUI *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis)
    {
        return pThis->handleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
        pThis = (TemperatureGUI *)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    }
    else
    {
        pThis = (TemperatureGUI *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis)
    {
        return pThis->handleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT TemperatureGUI::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_REFRESH_BUTTON)
        {
            updateTemperatureData();
        }
        break;
    }
    case WM_TIMER:
    {
        updateTemperatureData();
        break;
    }

    case WM_DESTROY:
    {
        KillTimer(hwnd_, 1);
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void TemperatureGUI::createControls(HWND hwnd)
{
    // Заголовок текущей температуры
    currentTempLabel_ = CreateWindow(
        (LPSTR) "STATIC", (LPSTR) "Current Temperature:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 20, 200, 25,
        hwnd, (HMENU)ID_CURRENT_TEMP_LABEL, hInstance_, NULL);
    // Значение текущей температуры
    currentTempValue_ = CreateWindow(
        (LPSTR) "STATIC", (LPSTR) "Loading...",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        220, 20, 100, 25,
        hwnd, (HMENU)ID_CURRENT_TEMP_VALUE, hInstance_, NULL);

    // Заголовок среднечасовой температуры
    hourlyAvgLabel_ = CreateWindow(
        (LPSTR) "STATIC", (LPSTR) "Hourly Average:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 50, 200, 25,
        hwnd, (HMENU)ID_HOURLY_AVG_LABEL, hInstance_, NULL);

    // Значение среднечасовой температуры
    hourlyAvgValue_ = CreateWindow(
        (LPCSTR) "STATIC", (LPCSTR) "Loading...",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        220, 50, 100, 25,
        hwnd, (HMENU)ID_HOURLY_AVG_VALUE, hInstance_, NULL);

    // Заголовок среднесуточной температуры
    dailyAvgLabel_ = CreateWindow(
        (LPCSTR) "STATIC", (LPCSTR) "Daily Average:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 80, 200, 25,
        hwnd, (HMENU)ID_DAILY_AVG_LABEL, hInstance_, NULL);

    // Значение среднесуточной температуры
    dailyAvgValue_ = CreateWindow(
        (LPCSTR) "STATIC", (LPCSTR) "Loading...",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        220, 80, 100, 25,
        hwnd, (HMENU)ID_DAILY_AVG_VALUE, hInstance_, NULL);

    // Кнопка обновления
    refreshButton_ = CreateWindow(
        (LPCSTR) "BUTTON", (LPCSTR) "Refresh",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        350, 20, 100, 30,
        hwnd, (HMENU)ID_REFRESH_BUTTON, hInstance_, NULL);

    // Список последних измерений
    measurementList_ = CreateWindow(
        WC_LISTVIEW, (LPCSTR) "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
        20, 120, 740, 400,
        hwnd, (HMENU)ID_MEASUREMENT_LIST, hInstance_, NULL);

    // Настройка колонок списка
    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = (LPSTR) "Time";
    lvc.cx = 200;
    ListView_InsertColumn(measurementList_, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = (LPSTR) "Temperature (°C)";
    lvc.cx = 150;
    ListView_InsertColumn(measurementList_, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = (LPSTR) "Status";
    lvc.cx = 150;
    ListView_InsertColumn(measurementList_, 2, &lvc);
}

void TemperatureGUI::updateTemperatureData()
{
    if (!monitor_)
    {
        // Если монитор еще не установлен, пробуем получить его
        monitor_ = &TemperatureMonitor::getInstance();
        if (!monitor_)
            return;
    }
    updateDisplay();
}

void TemperatureGUI::updateDisplay()
{
    // if (!monitor_) return;

    // Получаем данные с проверкой на валидность
    double currentTemp = monitor_->getCurrentTemperature();
    double hourlyAvg = monitor_->getHourlyAverage();
    double dailyAvg = monitor_->getDailyAverage();

    // Форматируем температуру с точностью до 2 знаков
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(2);

    // Обновление текущей температуры
    ss << currentTemp << L" °C";
    SetWindowTextW(currentTempValue_, ss.str().c_str());
    ss.str(L"");

    // Обновление среднечасовой температуры
    ss << hourlyAvg << L" °C";
    SetWindowTextW(hourlyAvgValue_, ss.str().c_str());
    ss.str(L"");

    // Обновление среднесуточной температуры
    ss << dailyAvg << L" °C";
    SetWindowTextW(dailyAvgValue_, ss.str().c_str());
    ss.str(L"");

    // Обновление списка измерений
    auto recentMeasurements = monitor_->getRecentMeasurements(20);
    ListView_DeleteAllItems(measurementList_);

    for (int i = 0; i < recentMeasurements.size(); ++i)
    {
        const auto &measurement = recentMeasurements[i];

        // Время измерения
        std::string timeStr = common::timeToString(measurement.first);
        std::wstring wTimeStr = stringToWString(timeStr);

        // Температура
        ss << std::fixed << std::setprecision(2) << measurement.second << L" °C";
        std::wstring wTempStr = ss.str();
        ss.str(L"");

        // Статус
        std::wstring wStatusStr;
        if (measurement.second >= 18 && measurement.second <= 28)
        {
            wStatusStr = L"Normal";
        }
        else if ((measurement.second >= 15 && measurement.second < 18) ||
                 (measurement.second > 28 && measurement.second <= 32))
        {
            wStatusStr = L"Warning";
        }
        else
        {
            wStatusStr = L"Critical";
        }

        // Добавление в список
        LVITEMA lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;

        std::string timeAnsi = wstringToString(wTimeStr);
        std::string tempAnsi = wstringToString(wTempStr);
        std::string statusAnsi = wstringToString(wStatusStr);

        lvi.pszText = const_cast<LPSTR>(timeAnsi.c_str());
        ListView_InsertItem(measurementList_, &lvi);

        ListView_SetItemText(measurementList_, i, 1, const_cast<LPSTR>(tempAnsi.c_str()));
        ListView_SetItemText(measurementList_, i, 2, const_cast<LPSTR>(statusAnsi.c_str()));
    }
}

std::string TemperatureGUI::wstringToString(const std::wstring &wstr)
{
    if (wstr.empty())
        return "";
    int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring TemperatureGUI::stringToWString(const std::string &str)
{
    if (str.empty())
        return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}