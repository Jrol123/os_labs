#ifndef NETWORK_COLLECTOR_H
#define NETWORK_COLLECTOR_H

#include "common.h"
#include <string>
#include <thread>
#include <atomic>
#include <functional>

class NetworkCollector {
public:
    NetworkCollector(const std::string& address = "127.0.0.1", int port = 5555);
    ~NetworkCollector();
    
    bool start();
    void stop();
    
    void setDataHandler(std::function<void(const std::string&)> handler) {
        data_handler_ = handler;
    }
    
    bool isRunning() const { return running_; }

private:
    void run();
    bool connectToServer();
    
    std::string server_address_;
    int server_port_;
    std::atomic<bool> running_{false};
    std::thread collector_thread_;
    std::function<void(const std::string&)> data_handler_;
};

#endif