#include "Client.h"

bool Client::ConnectToServer()
{
    InitializeAddress(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0)
    {
        perror("Invalid address");
        CloseSocket();
        return false;
    }

    if (connect(socketFd, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        perror("Connection failed");
        CloseSocket();
        return false;
    }
    std::cout << "Connected to server" << std::endl;
    return true;
}

void Client::Run()
{
    if (!CreateSocket())
    {
        return;
    }

    if (!ConnectToServer())
    {
        return;
    }

    int clientNum;
    std::cout << "Enter a number between 1 and 100: ";
    std::cin >> clientNum;
    if (clientNum < 1 || clientNum > 100)
    {
        std::cerr << "Invalid number. Exiting." << std::endl;
        CloseSocket();
        return;
    }

    std::ostringstream oss;
    oss << CLIENT_NAME << "," << clientNum;
    if (!SendData(oss.str()))
    {
        CloseSocket();
        return;
    }

    std::string response = ReceiveData();
    if (response.empty())
    {
        CloseSocket();
        return;
    }

    std::istringstream iss(response);
    std::string serverName;
    int serverNum;
    if (!std::getline(iss, serverName, ',') || !(iss >> serverNum))
    {
        std::cerr << "Failed to parse server response" << std::endl;
        CloseSocket();
        return;
    }

    std::cout << "\nClient: " << CLIENT_NAME << "\n"
              << "Server: " << serverName << "\n"
              << "Client number: " << clientNum << "\n"
              << "Server number: " << serverNum << "\n"
              << "Sum: " << clientNum + serverNum << std::endl;

    CloseSocket();
}
