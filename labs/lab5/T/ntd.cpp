// ����������� ������, ����������� ��������� ���������� �� HTML-���������

#include <iostream> /* std::cout */
#include <string>   /* std::string */
#include <sstream>  /* std::stringbuf */

#include <stdlib.h> /* atoi */
#include <string.h> /* memset */

#if defined(_WIN32)
#include <winsock2.h> /* socket */
#include <ws2tcpip.h> /* ipv6 */
#else
#include <sys/socket.h> /* socket */
#include <netinet/in.h> /* socket */
#include <arpa/inet.h>  /* socket */
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// �������� ������� �� �������� � ��
#define READ_WAIT_MS 50

// ������� �������� ������� �� ������ � ��������, ���������� � ��������� �����
class SocketWorker
{
public:
    SocketWorker() : m_socket(INVALID_SOCKET) {}
    ~SocketWorker() { CloseMySocket(); }
    /// ���������� ��� ������
    static int ErrorCode()
    {
#if defined(_WIN32)
        return WSAGetLastError();
#else
        return errno;
#endif
    }

protected:
    void CloseMySocket()
    {
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    void CloseSocket(SOCKET sock)
    {
        if (sock == INVALID_SOCKET)
            return;
#if defined(_WIN32)
        shutdown(sock, SD_SEND);
        closesocket(sock);
#else
        shutdown(sock, SHUT_WR);
        close(sock);
#endif
    }
    SOCKET m_socket;
};

// ������ �������, ���������� �� ��� ������ �� UDP
class NtdClient : public SocketWorker
{
public:
    // ������������ UDP-����� �� IPv6 ��� ��������� ��������� �� �������
    SOCKET Connect(const std::string &interface_ipv6, short int port)
    {
        if (m_socket != INVALID_SOCKET)
        {
            std::cout << "Already connected!" << std::endl;
            return m_socket;
        }
        // ������� IPv6 UDP-�����
        m_socket = socket(AF_INET6, SOCK_DGRAM, 0);
        if (m_socket == INVALID_SOCKET)
        {
            std::cout << "Cant open socket: " << ErrorCode() << std::endl;
            return INVALID_SOCKET;
        }
        // ������ ����� �� ��������� ����� � ����
        sockaddr_in6 local_addr;
        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin6_family = AF_INET6;
        if (inet_pton(AF_INET6, interface_ipv6.c_str(), &local_addr.sin6_addr) != 1)
        {
            std::cout << "Cant convert IPv6 device address with error code: "
                      << ErrorCode() << std::endl;
            CloseMySocket();
            return INVALID_SOCKET;
        }
        local_addr.sin6_port = htons(port);
        if (bind(m_socket, (struct sockaddr *)&local_addr, sizeof(local_addr)))
        {
            std::cout << "Failed to bind: " << ErrorCode() << std::endl;
            CloseMySocket();
            return INVALID_SOCKET;
        }
        return m_socket;
    }
    // ������ �� ������ ������ (����������� �����!)
    int Read(char *data, size_t max_data_read)
    {
        if (!max_data_read)
            return 0;
        // ������� ������ � ������
        int readd = recv(m_socket, data, (int)max_data_read, 0);
        if (!readd || readd == SOCKET_ERROR)
        {
            std::cout << "Nothing read from device!" << std::endl;
            return -1;
        }
        return readd;
    }
};

// ������, �������� HTML-�������� � ������� �� �������
class HTTPServer : public SocketWorker
{
public:
    // ������������ ����� �� ��������� �����������
    SOCKET Listen(const std::string &interface_ip, short int port)
    {
        // ������� ����� ipv4
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket == INVALID_SOCKET)
        {
            std::cout << "Cant open socket: " << ErrorCode() << std::endl;
            return INVALID_SOCKET;
        }
        // ������ ����� �� ����� � ����
        sockaddr_in local_addr;
        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = inet_addr(interface_ip.c_str());
        local_addr.sin_port = htons(port);
        if (bind(m_socket, (struct sockaddr *)&local_addr, sizeof(local_addr)))
        {
            std::cout << "Failed to bind: " << ErrorCode() << std::endl;
            CloseMySocket();
            return INVALID_SOCKET;
        }
        // ��������� ��������� �� ������
        if (listen(m_socket, SOMAXCONN))
        {
            std::cout << "Failed to start listen: " << ErrorCode() << std::endl;
            CloseMySocket();
            return INVALID_SOCKET;
        }
        return m_socket;
    }
    // ���������� ����������� ������� (��������), ��������� ��� ������ � �������
    void ProcessClient(const char *device_str)
    {
        // ��������� �������� ����������
        SOCKET client_socket = accept(m_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET)
        {
            std::cout << "Error accepting client: " << ErrorCode() << std::endl;
            CloseSocket(client_socket);
            return;
        }
        // ����� �� ������ � ���� ��������?
        // (����������� �������� ���� ������� ��� ����������� �����)
        struct pollfd polstr;
        memset(&polstr, 0, sizeof(polstr));
        polstr.fd = client_socket;
        polstr.events |= POLLIN;
#ifdef _WIN32
        int ret = WSAPoll(&polstr, 1, READ_WAIT_MS);
#else
        int ret = poll(&polstr, 1, READ_WAIT_MS);
#endif
        // �� ����� - ��������� �����
        if (ret <= 0)
        {
            CloseSocket(client_socket);
            return;
        }
        // ���������, ��� ������ ��� ������ (����������� �����!!)
        int result = recv(client_socket, m_input_buf, sizeof(m_input_buf), 0);
        if (result == SOCKET_ERROR)
        {
            std::cout << "Error on client receive: " << result << std::endl;
            CloseSocket(client_socket);
            return;
        }
        else if (result == 0)
        {
            std::cout << "Client closed connection before getting any data!" << std::endl;
            CloseSocket(client_socket);
            return;
        }
        // ���� ������� ������ ����� �������
        std::stringstream response;
        // ���� ������� HTML-�������� � �������
        std::stringstream response_body; // ���� ������
        // TODO: ������� ������ ������ ������������� ��������� ������� (m_input_buf)
        // � �� �� ������ ��������� �����. �� ��� - ������ ������ )
        response_body << "<html>\n<head>"
                      << "\t<meta http-equiv=\"Refresh\" content=\"1\" />\n"
                      << "\t<title>Temperature Device Server</title>\n"
                      << "</head>\n\t<body>\n"
                      << "\t\t<h1>Temperature Device Server</h1>\n"
                      << "\t\t<p>Current device data: <b>"
                      << (strlen(device_str) ? device_str : "No data provided!")
                      << "</b></p>\n"
                      << "\t</body>\n</html>";
        // ��������� ���� ����� ������ � �����������
        response << "HTTP/1.0 200 OK\r\n"
                 << "Version: HTTP/1.1\r\n"
                 << "Content-Type: text/html; charset=utf-8\r\n"
                 << "Content-Length: " << response_body.str().length()
                 << "\r\n\r\n"
                 << response_body.str();
        // ���������� ����� �������
        result = send(client_socket, response.str().c_str(), (int)response.str().length(), 0);
        if (result == SOCKET_ERROR)
        {
            // ��������� ������ ��� �������� ������
            std::cout << "Failed to send responce to client: " << ErrorCode() << std::endl;
        }
        // ��������� ���������� � ��������
        CloseSocket(client_socket);
        std::cout << "Answered to client!" << std::endl;
    }

private:
    // ����� ��� ������ ������� ��������
    char m_input_buf[1024];
};

// �������� ���������
int main(int argc, char **argv)
{
    if (argc < 5)
    {
        std::cout << "Usage: ntd ipv6dev_addr dev_port ipv4srv_addr srv_port" << std::endl;
        return -1;
    }
    // �������������� ���������� ������� (��� Windows)
#if defined(_WIN32)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
    // ���������� SIGPIPE ������
    // ����� ��������� �� ��������������� ��� ������� ������ � �������� �����
    signal(SIGPIPE, SIG_IGN);
#endif
    // ������� ������ � ������ � �������������� ��
    NtdClient cli;
    HTTPServer srv;
    SOCKET client_socket = cli.Connect(argv[1], atoi(argv[2]));
    SOCKET server_socket = srv.Listen(argv[3], atoi(argv[4]));
    if (client_socket == INVALID_SOCKET || server_socket == INVALID_SOCKET)
    {
        std::cout << "Terminating..." << std::endl;
#if defined(_WIN32)
        WSACleanup();
#endif
        return -1;
    }
    // ����� ����� ������� �� ���� ������� - ������� UDP � ������� TCP
    struct pollfd polstr[2];
    memset(polstr, 0, sizeof(polstr));
    polstr[0].fd = client_socket;
    polstr[0].events |= POLLIN;
    polstr[1].fd = server_socket;
    polstr[1].events |= POLLIN;

    // ����� ��� ��������� ������ �� �������
    char server_buf[255];
    memset(server_buf, 0, sizeof(server_buf));
    // ���� �������� ������� �� �������
    for (;;)
    {
        int ret = 0;
#ifdef _WIN32
        ret = WSAPoll(polstr, 2, -1);
#else
        ret = poll(polstr, 2, -1);
#endif
        // ������ ������
        if (ret <= 0)
        {
            std::cout << "Error on poll: " << SocketWorker::ErrorCode() << std::endl;
            continue;
        }
        // ��������� ���������� �������
        if (polstr[0].revents & POLLIN)
        {
            // ����� �������� ������ � ������� - ������ ��
            if (cli.Read(server_buf, sizeof(server_buf) - 1) > 0)
                std::cout << "MSG from device:" << server_buf << std::endl;
        }
        if (polstr[1].revents & POLLIN)
        {
            // ���� HTTP-������ - ���������� ��� ��������
            srv.ProcessClient(server_buf);
        }
    }
    // ���� �� ������� �� �������, �� � ����� ����� ������������� ������� ������, �������� �� ������ Poll
    // ���������������� ���������� ������� (��� Windows)
#if defined(_WIN32)
    WSACleanup();
#endif
    return 0;
}