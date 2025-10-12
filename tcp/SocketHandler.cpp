#include "SocketHandler.h"

void SocketHandler::InitializeAddress(int port)
{
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;
}

bool SocketHandler::CreateSocket()
{
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0)
    {
        perror("Socket creation failed");
        return false;
    }
    std::cout << "Socket created successfully" << std::endl;
    return true;
}

bool SocketHandler::SendData(const std::string &message) const
{
    if (send(socketFd, message.c_str(), message.size(), 0) < 0)
    {
        perror("Send failed");
        return false;
    }
    std::cout << "Sent: " << message << std::endl;
    return true;
}

std::string SocketHandler::ReceiveData() const
{
    char buffer[MAX_BUFFER];
    int bytesReceived = recv(socketFd, buffer, MAX_BUFFER, 0);
    if (bytesReceived < 0)
    {
        perror("Receive failed");
        return "";
    }
    buffer[bytesReceived] = '\0';
    return std::string(buffer);
}

void SocketHandler::CloseSocket() const
{
    if (socketFd > 0)
    {
        close(socketFd);
        std::cout << "Socket closed" << std::endl;
    }
}
