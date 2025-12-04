#include "network_publisher.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

NetworkPublisher::NetworkPublisher(int port) : port_(port)
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

NetworkPublisher::~NetworkPublisher()
{
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool NetworkPublisher::start()
{
    if (running_)
        return true;

    running_ = true;
    server_thread_ = std::thread(&NetworkPublisher::run, this);
    return true;
}

void NetworkPublisher::stop()
{
    running_ = false;
    if (server_thread_.joinable())
    {
        server_thread_.join();
    }
}

void NetworkPublisher::publish(const std::string &data)
{
    // Данные публикуются через callback в основном потоке
    if (data_callback_)
    {
        std::string current_data = data_callback_();
        // Можно добавить логирование или обработку
    }
}

void NetworkPublisher::run()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

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

    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Listen failed" << std::endl;
#ifdef _WIN32
        closesocket(server_fd);
#else
        close(server_fd);
#endif
        return;
    }

    std::cout << "Publisher listening on port " << port_ << std::endl;

    while (running_)
    {
        sockaddr_in client_addr;
#ifdef _WIN32
        int client_len = sizeof(client_addr);
#else
        socklen_t client_len = sizeof(client_addr);
#endif

        SOCKET client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET)
        {
            if (running_)
            {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }

        // Отправляем текущие данные клиенту
        if (data_callback_)
        {
            std::string data = data_callback_();
#ifdef _WIN32
            send(client_socket, data.c_str(), (int)data.length(), 0);
#else
            write(client_socket, data.c_str(), data.length());
#endif
        }

#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
    }

#ifdef _WIN32
    closesocket(server_fd);
#else
    close(server_fd);
#endif
}