#ifndef NETWORK_PUBLISHER_H
#define NETWORK_PUBLISHER_H

#include "common.h"
#include <string>
#include <thread>
#include <atomic>
#include <functional>

class NetworkPublisher
{
public:
    NetworkPublisher(int port = 5555);
    ~NetworkPublisher();

    bool start();
    void stop();
    void publish(const std::string &data);

    void setDataCallback(std::function<std::string()> callback)
    {
        data_callback_ = callback;
    }

    bool isRunning() const { return running_; }
    int getPort() const { return port_; }

private:
    void run();
    void handleClient(int client_socket);

    int port_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    std::function<std::string()> data_callback_;
};

#endif