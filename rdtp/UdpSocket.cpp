#include "UdpSocket.h"
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <chrono>

UdpSocket::UdpSocket() {
    m_socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    std::srand(std::time(nullptr));
}

UdpSocket::~UdpSocket() {
    if (m_socketFd >= 0) close(m_socketFd);
}

bool UdpSocket::Bind(int port) {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    return bind(m_socketFd, (struct sockaddr*)&addr, sizeof(addr)) >= 0;
}

void UdpSocket::SetDestination(const std::string& ip, int port) {
    std::memset(&m_destAddr, 0, sizeof(m_destAddr));
    m_destAddr.sin_family = AF_INET;
    m_destAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &m_destAddr.sin_addr);
}

bool UdpSocket::SendUnreliable(const Packet& pkt, double lossProb, int delayMs) {
    double r = (double)std::rand() / RAND_MAX;
    if (r < lossProb) {
        return true;
    }

    if (delayMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % delayMs));
    }

    return Send(pkt);
}

bool UdpSocket::Send(const Packet& pkt) {
    int sent = sendto(m_socketFd, &pkt, sizeof(pkt), 0,
                      (struct sockaddr*)&m_destAddr, sizeof(m_destAddr));
    return sent >= 0;
}

int UdpSocket::Recv(Packet& pkt, sockaddr_in* sender) {
    socklen_t len = sizeof(sockaddr_in);
    sockaddr_in tempSender;
    int bytes = recvfrom(m_socketFd, &pkt, sizeof(pkt), 0,
                         (struct sockaddr*)&tempSender, &len);
    if (sender) *sender = tempSender;
    return bytes;
}

void UdpSocket::SetRecvTimeout(int ms) {
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    setsockopt(m_socketFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}