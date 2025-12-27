#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "RdtHeader.h"

class UdpSocket {
public:
    UdpSocket();
    ~UdpSocket();

    bool Bind(int port);
    void SetDestination(const std::string& ip, int port);
    bool SendUnreliable(const Packet& pkt, double lossProb, int delayMs);
    bool Send(const Packet& pkt);
    int Recv(Packet& pkt, sockaddr_in* sender = nullptr);
    void SetRecvTimeout(int ms);

private:
    int m_socketFd;
    sockaddr_in m_destAddr;
};

#endif