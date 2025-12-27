#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

class UdpPingerClient
{
private:
    int m_socketFd;
    struct sockaddr_in m_serverAddress;
    const int m_totalPackets = 10;
    const int m_timeoutSec = 1;

    void GetCurrentTime(struct timeval &t) { gettimeofday(&t, NULL); }

    double CalculateRtt(struct timeval start, struct timeval end)
    {
        double startSec = start.tv_sec + start.tv_usec / 1000000.0;
        double endSec = end.tv_sec + end.tv_usec / 1000000.0;
        return endSec - startSec;
    }

public:
    UdpPingerClient(const std::string &serverIp, int serverPort)
    {
        m_socketFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (m_socketFd < 0)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        memset(&m_serverAddress, 0, sizeof(m_serverAddress));
        m_serverAddress.sin_family = AF_INET;
        m_serverAddress.sin_port = htons(serverPort);
        if (inet_pton(AF_INET, serverIp.c_str(), &m_serverAddress.sin_addr) <= 0)
        {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            exit(EXIT_FAILURE);
        }

        struct timeval tv;
        tv.tv_sec = m_timeoutSec;
        tv.tv_usec = 0;
        if (setsockopt(m_socketFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            perror("Error setting timeout");
            exit(EXIT_FAILURE);
        }
    }

    ~UdpPingerClient()
    {
        if (m_socketFd >= 0)
        {
            close(m_socketFd);
        }
    }

    void Run()
    {
        std::cout << "Starting UDP Ping to server..." << std::endl;

        char buffer[1024];
        struct sockaddr_in fromAddr;
        socklen_t fromLen = sizeof(fromAddr);

        for (int seq = 1; seq <= m_totalPackets; ++seq)
        {
            struct timeval sendTime, receiveTime;

            GetCurrentTime(sendTime);

            long long timestampMs = (long long) sendTime.tv_sec * 1000 + sendTime.tv_usec / 1000;
            std::string message = "Ping " + std::to_string(seq) + " " + std::to_string(timestampMs);

            ssize_t sentBytes = sendto(m_socketFd, message.c_str(), message.length(), 0,
                                       (const struct sockaddr *) &m_serverAddress, sizeof(m_serverAddress));

            if (sentBytes < 0)
            {
                perror("Send failed");
                continue;
            }

            memset(buffer, 0, sizeof(buffer));
            ssize_t receivedBytes =
                    recvfrom(m_socketFd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &fromAddr, &fromLen);

            if (receivedBytes > 0)
            {
                GetCurrentTime(receiveTime);
                double rtt = CalculateRtt(sendTime, receiveTime);

                buffer[receivedBytes] = '\0'; // Null-terminate string
                printf("Ответ от сервера: %s, RTT = %.3f сек\n", buffer, rtt);
            } else
            {
                std::cout << "Request timed out" << std::endl;
            }
        }
    }
};

int main(int argc, char *argv[])
{
    std::string serverIp = "127.0.0.1";
    int serverPort = 12000;

    if (argc > 1)
        serverIp = argv[1];
    if (argc > 2)
        serverPort = std::atoi(argv[2]);

    UdpPingerClient client(serverIp, serverPort);
    client.Run();

    return 0;
}
