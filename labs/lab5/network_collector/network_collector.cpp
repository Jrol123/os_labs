#include "network_collector.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

NetworkCollector::NetworkCollector(const std::string &address, int port)
    : server_address_(address), server_port_(port)
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

NetworkCollector::~NetworkCollector()
{
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool NetworkCollector::start()
{
    if (running_)
        return true;

    running_ = true;
    collector_thread_ = std::thread(&NetworkCollector::run, this);
    return true;
}

void NetworkCollector::stop()
{
    running_ = false;
    if (collector_thread_.joinable())
    {
        collector_thread_.join();
    }
}

void NetworkCollector::run()
{
    while (running_)
    {
        if (connectToServer())
        {
            // Успешно подключились и получили данные
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else
        {
            // Ждем перед повторной попыткой
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

bool NetworkCollector::connectToServer()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port_);

    if (inet_pton(AF_INET, server_address_.c_str(), &serv_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address" << std::endl;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return false;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return false;
    }

    char buffer[1024] = {0};
#ifdef _WIN32
    int bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
#else
    int bytes_read = read(sock, buffer, sizeof(buffer) - 1);
#endif

    if (bytes_read > 0)
    {
        std::string data(buffer, bytes_read);
        if (data_handler_)
        {
            data_handler_(data);
        }
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return true;
    }

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return false;
}