#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "temperature_monitor.h"
#include <string>
#include <thread>
#include <atomic>
#include <map>

class HTTPServer
{
public:
    HTTPServer(TemperatureMonitor &monitor);
    ~HTTPServer();

    bool start(int port = 8080);
    void stop();
    bool isRunning() const { return running_; }

private:
    void run();
    std::string generateHTMLResponse();
    // std::string loadTemplate();
    std::string generateRecentMeasurementsHTML();
    std::string getTemperatureIcon(double temperature);

    bool loadTemplateFromFile();

    TemperatureMonitor &monitor_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    int port_;
    std::string html_template_;
};

#endif