#include "http_server.h"
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
    std::stringstream html;

    // Получаем реальные данные из монитора
    double current_temp = monitor_.getCurrentTemperature();
    double hourly_avg = monitor_.getHourlyAverage();
    double daily_avg = monitor_.getDailyAverage();
    auto recent_measurements = monitor_.getRecentMeasurements(5);

    html << R"(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Temperature Monitor</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; color: #333; border-bottom: 2px solid #eee; padding-bottom: 20px; margin-bottom: 30px; }
        .section { margin-bottom: 30px; padding: 20px; border: 1px solid #ddd; border-radius: 5px; }
        .current-temp { font-size: 2em; color: #e74c3c; text-align: center; margin: 20px 0; }
        .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; }
        .stat-card { background: #f8f9fa; padding: 15px; border-radius: 5px; border-left: 4px solid #3498db; }
        .stat-value { font-size: 1.5em; font-weight: bold; color: #2c3e50; }
        .last-update { text-align: center; color: #7f8c8d; font-style: italic; margin-top: 30px; }
        table { width: 100%; border-collapse: collapse; margin-top: 10px; }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background-color: #f2f2f2; }
        tr:hover { background-color: #f5f5f5; }
        .status-normal { color: #27ae60; }
        .status-warning { color: #f39c12; }
        .status-critical { color: #e74c3c; }
    </style>
</head>
<body>
    <div class='container'>
        <div class='header'>
            <h1>🌡️ Temperature Monitoring System</h1>
            <p>Real-time temperature data and statistics</p>
        </div>
    )";

    // Current temperature section
    html << R"(
        <div class='section'>
            <h2>📊 Current Temperature</h2>
            <div class='current-temp'>)";

    if (current_temp > 30)
    {
        html << "🔥 " << current_temp << "°C 🌡️";
    }
    else if (current_temp < 15)
    {
        html << "❄️ " << current_temp << "°C 🌡️";
    }
    else
    {
        html << "🌡️ " << current_temp << "°C";
    }

    html << R"(</div>
            <div style='text-align: center; color: #666;'>
                Last measurement: )"
                //! TODO: Нужно, чтобы время постоянно обновлялось!
         << __TIME__ << R"(
            </div>
        </div>
    )";

    // Statistics section
    html << R"(
        <div class='section'>
            <h2>📈 Temperature Statistics</h2>
            <div class='stats-grid'>
                <div class='stat-card'>
                    <h3>Current</h3>
                    <div class='stat-value'>)"
         << current_temp << "°C</div>"
         << R"(<div style='color: #666; font-size: 0.9em;'>Latest reading</div>
                </div>
                <div class='stat-card'>
                    <h3>Hourly Average</h3>
                    <div class='stat-value'>)"
         << hourly_avg << "°C</div>"
         << R"(<div style='color: #666; font-size: 0.9em;'>Last hour</div>
                </div>
                <div class='stat-card'>
                    <h3>Daily Average</h3>
                    <div class='stat-value'>)"
         << daily_avg << "°C</div>"
         << R"(<div style='color: #666; font-size: 0.9em;'>Last 24 hours</div>
                </div>
                <div class='stat-card'>
                    <h3>Measurements</h3>
                    <div class='stat-value'>)"
         << recent_measurements.size() << "</div>"
         << R"(<div style='color: #666; font-size: 0.9em;'>Recent count</div>
                </div>
            </div>
            
            <h3 style='margin-top: 30px;'>Recent Measurements</h3>
            <table>
                <thead>
                    <tr>
                        <th>Time</th>
                        <th>Temperature</th>
                        <th>Status</th>
                    </tr>
                </thead>
                <tbody>
    )";

    // Add recent measurements
    for (const auto &measurement : recent_measurements)
    {
        std::string time_str = common::timeToString(measurement.first);
        double temp = measurement.second;

        html << "<tr><td>" << time_str << "</td><td>" << temp << "°C</td><td class='";

        if (temp >= 18 && temp <= 28)
        {
            html << "status-normal'>🟢 Normal";
        }
        else if (temp >= 15 && temp < 18 || temp > 28 && temp <= 32)
        {
            html << "status-warning'>🟡 Warning";
        }
        else
        {
            html << "status-critical'>🔴 Critical";
        }

        html << "</td></tr>";
    }

    html << R"(
                </tbody>
            </table>
        </div>
        <div class='last-update'>
            Last updated: )"
         << __TIME__ << " " << __DATE__ << R"(
        </div>
    </div>
    <script>
        // Auto-refresh every 5 seconds
        setTimeout(() => location.reload(), 5000);
    </script>
</body>
</html>
    )";

    return html.str();
}