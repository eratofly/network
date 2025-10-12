#ifndef SOCKET_HANDLER_H
#define SOCKET_HANDLER_H

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>

class SocketHandler
{
public:
    int socketFd = 0;
    struct sockaddr_in address = {};
    static constexpr int MAX_BUFFER = 256;

    void InitializeAddress(int port);
    bool CreateSocket();
    bool SendData(const std::string &message) const;
    std::string ReceiveData() const;
    void CloseSocket() const;
};

#endif // SOCKET_HANDLER_H
