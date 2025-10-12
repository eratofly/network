#include "Server.h"

bool Server::StartListening()
{
    if (!CreateSocket())
    {
        return false;
    }

    InitializeAddress(PORT);

    if (bind(socketFd, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        CloseSocket();
        return false;
    }
    std::cout << "Socket bound to port " << PORT << std::endl;

    if (listen(socketFd, 5) < 0)
    {
        perror("Listen failed");
        CloseSocket();
        return false;
    }
    std::cout << "Server is listening..." << std::endl;
    return true;
}

void Server::HandleClient()
{
    struct sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);

    int clientSocket = accept(socketFd, (struct sockaddr *) &clientAddress, &clientLen);
    if (clientSocket < 0)
    {
        perror("Accept failed");
        return;
    }
    std::cout << "Client connected" << std::endl;

    SocketHandler clientHandler;
    clientHandler.socketFd = clientSocket;

    std::string message = clientHandler.ReceiveData();
    if (message.empty())
    {
        clientHandler.CloseSocket();
        return;
    }

    std::istringstream iss(message);
    std::string clientName;
    int clientNum;
    if (!std::getline(iss, clientName, ',') || !(iss >> clientNum))
    {
        std::cerr << "Failed to parse client message" << std::endl;
        clientHandler.CloseSocket();
        return;
    }

    std::cout << "\nClient name: " << clientName << "\n"
              << "Server name: " << SERVER_NAME << "\n"
              << "Client number: " << clientNum << "\n"
              << "Server number: " << SERVER_NUMBER << "\n"
              << "Sum: " << clientNum + SERVER_NUMBER << std::endl;

    if (clientNum < 1 || clientNum > 100)
    {
        std::cout << "Invalid number received. Shutting down server." << std::endl;
        clientHandler.CloseSocket();
        CloseSocket();
        return;
    }

    std::ostringstream oss;
    oss << SERVER_NAME << "," << SERVER_NUMBER;
    if (!clientHandler.SendData(oss.str()))
    {
        clientHandler.CloseSocket();
        return;
    }

    clientHandler.CloseSocket();
    std::cout << "Client socket closed" << std::endl;
}

void Server::Run()
{
    if (!StartListening())
    {
        return;
    }

    while (true)
    {
        HandleClient();
    }
}
