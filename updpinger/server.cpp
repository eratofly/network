#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class UdpPingerServer {
private:
    int m_socketFd;
    struct sockaddr_in m_serverAddress{};
    int m_port;
    const int m_dropProbability = 30;

public:
    explicit UdpPingerServer(int port) : m_port(port) {
        std::srand(std::time(nullptr));

        m_socketFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (m_socketFd < 0) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        memset(&m_serverAddress, 0, sizeof(m_serverAddress));
        m_serverAddress.sin_family = AF_INET;
        m_serverAddress.sin_addr.s_addr = INADDR_ANY;
        m_serverAddress.sin_port = htons(m_port);

        if (bind(m_socketFd, (const struct sockaddr *)&m_serverAddress, sizeof(m_serverAddress)) < 0) {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }

        std::cout << "UDP Server started on port " << m_port << std::endl;
    }

    ~UdpPingerServer() {
        if (m_socketFd >= 0) {
            close(m_socketFd);
        }
    }

    void Run() {
        char buffer[1024];
        struct sockaddr_in clientAddress;
        socklen_t clientLen = sizeof(clientAddress);

        std::cout << "Waiting for data..." << std::endl;

        while (true) {
            memset(buffer, 0, sizeof(buffer));

            ssize_t receivedBytes = recvfrom(m_socketFd, buffer, sizeof(buffer) - 1, 0,
                                             (struct sockaddr *)&clientAddress, &clientLen);

            if (receivedBytes < 0) {
                perror("Recvfrom failed");
                continue;
            }

            buffer[receivedBytes] = '\0';

            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddress.sin_addr, clientIp, INET_ADDRSTRLEN);
            std::cout << "Received from " << clientIp << ":" << ntohs(clientAddress.sin_port)
                      << " -> " << buffer << std::endl;

            int randomVal = std::rand() % 100;
            if (randomVal < m_dropProbability) {
                std::cout << "--> Packet dropped (Simulated loss)" << std::endl;
                continue;
            }

            ssize_t sentBytes = sendto(m_socketFd, buffer, receivedBytes, 0,
                                       (const struct sockaddr *)&clientAddress, clientLen);

            if (sentBytes < 0) {
                perror("Sendto failed");
            } else {
                std::cout << "--> Echo reply sent" << std::endl;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    int port = 12000;

    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    UdpPingerServer server(port);
    server.Run();

    return 0;
}