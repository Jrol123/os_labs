#include "http_server.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

#ifdef _WIN32
#define GET_ERROR() WSAGetLastError()
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <errno.h>
#define GET_ERROR() errno
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

HTTPServer::HTTPServer(TemperatureMonitor &monitor)
    : monitor_(monitor), port_(8080)
{
}

HTTPServer::~HTTPServer()
{
    stop();
}

bool HTTPServer::start(int port)
{
    if (running_)
        return true;

#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed" << result << std::endl;
        return false;
    }
#endif

    if (!loadTemplateFromFile())
    {
        std::cerr << "Failed to load HTML template" << std::endl;
        return false;
    }

    port_ = port;
    running_ = true;
    server_thread_ = std::thread(&HTTPServer::run, this);
    return true;
}

void HTTPServer::stop()
{
    if (running_)
    {
        running_ = false;
        if (server_thread_.joinable())
        {
            server_thread_.join();
        }
    }
}

void HTTPServer::run()
{
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket. Error: " << GET_ERROR() << std::endl;
        return;
    }

    // Set socket options
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Bind failed" << std::endl;
#ifdef _WIN32
        closesocket(server_fd);
#else
        close(server_fd);
#endif
        return;
    }

    if (listen(server_fd, 10) < 0)
    {
        std::cerr << "Listen failed" << std::endl;
#ifdef _WIN32
        closesocket(server_fd);
#else
        close(server_fd);
#endif
        return;
    }

    std::cout << "HTTP Server running on port " << port_ << std::endl;

    while (running_)
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        SOCKET client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == INVALID_SOCKET)
        {
            if (running_)
            {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }

        // Read request
        char buffer[4096] = {0};
#ifdef _WIN32
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
#else
        int bytes_received = read(client_fd, buffer, sizeof(buffer) - 1);
#endif

        if (bytes_received <= 0)
        {
            std::cerr << "Failed to read from client" << std::endl;
#ifdef _WIN32
            closesocket(client_fd);
#else
            close(client_fd);
#endif
            continue;
        }

        // Generate response
        std::string response = generateHTMLResponse();

        // Send response
        std::string http_response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: " +
            std::to_string(response.length()) + "\r\n"
                                                "Connection: close\r\n"
                                                "\r\n" +
            response;

#ifdef _WIN32
        int bytes_sent = send(client_fd, http_response.c_str(), (int)http_response.length(), 0);
#else
        int bytes_sent = write(client_fd, http_response.c_str(), http_response.length());
#endif

        if (bytes_sent < 0)
        {
            std::cerr << "Failed to send response to client" << std::endl;
        }

#ifdef _WIN32
        closesocket(client_fd);
#else
        close(client_fd);
#endif
    }

#ifdef _WIN32
    closesocket(server_fd);
#else
    close(server_fd);
#endif
}

std::string HTTPServer::generateHTMLResponse()
{
    // Получаем реальные данные из монитора
    double current_temp = monitor_.getCurrentTemperature();
    double hourly_avg = monitor_.getHourlyAverage();
    double daily_avg = monitor_.getDailyAverage();
    auto recent_measurements = monitor_.getRecentMeasurements(5);

    // Создаем копию шаблона
    std::string response = html_template_;

    // Заменяем плейсхолдеры на реальные данные
    size_t pos = 0;

    // Замена плейсхолдеров
    auto replacePlaceholder = [&](const std::string &placeholder, const std::string &value)
    {
        while ((pos = response.find(placeholder, pos)) != std::string::npos)
        {
            response.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
        pos = 0; // Сбрасываем позицию для следующего поиска
    };

    // Основные данные
    replacePlaceholder("{{CURRENT_TEMP_ICON}}", getTemperatureIcon(current_temp));
    replacePlaceholder("{{CURRENT_TEMP}}", std::to_string(current_temp));
    replacePlaceholder("{{HOURLY_AVG}}", std::to_string(hourly_avg));
    replacePlaceholder("{{DAILY_AVG}}", std::to_string(daily_avg));
    replacePlaceholder("{{MEASUREMENTS_COUNT}}", std::to_string(recent_measurements.size()));

    // Временные данные
    auto now = common::getCurrentTime();
    std::string current_time = common::timeToString(now);
    replacePlaceholder("{{LAST_MEASUREMENT_TIME}}", current_time);
    replacePlaceholder("{{LAST_UPDATE}}", current_time);

    // Таблица измерений
    replacePlaceholder("{{RECENT_MEASUREMENTS}}", generateRecentMeasurementsHTML());

    return response;
}

bool HTTPServer::loadTemplateFromFile()
{
    std::ifstream file("http_server/template.html");
    if (!file.is_open())
    {
        std::cerr << "Cannot open template.html file" << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    html_template_ = buffer.str();
    file.close();

    if (html_template_.empty())
    {
        std::cerr << "Template file is empty" << std::endl;
        return false;
    }

    std::cout << "HTML template loaded successfully" << std::endl;
    return true;
}

std::string HTTPServer::getTemperatureIcon(double temperature)
{
    if (temperature > 30)
    {
        return "🔥";
    }
    else if (temperature < 15)
    {
        return "❄️";
    }
    else
    {
        return "🌡️";
    }
}

std::string HTTPServer::generateRecentMeasurementsHTML()
{
    auto recent_measurements = monitor_.getRecentMeasurements(5);
    std::stringstream ss;

    for (const auto &measurement : recent_measurements)
    {
        std::string time_str = common::timeToString(measurement.first);
        double temp = measurement.second;

        ss << "<tr><td>" << time_str << "</td><td>" << temp << "°C</td><td class='";

        if (temp >= 18 && temp <= 28)
        {
            ss << "status-normal'>🟢 Normal";
        }
        else if ((temp >= 15 && temp < 18) || (temp > 28 && temp <= 32))
        {
            ss << "status-warning'>🟡 Warning";
        }
        else
        {
            ss << "status-critical'>🔴 Critical";
        }

        ss << "</td></tr>";
    }

    return ss.str();
}
