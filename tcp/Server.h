#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <sstream>
#include "SocketHandler.h"

class Server : public SocketHandler
{
public:
    static constexpr int PORT = 5555;
    static constexpr int SERVER_NUMBER = 50;
    static constexpr auto SERVER_NAME = "Server Lalala";

    bool StartListening();
    void HandleClient();
    void Run();
};

#endif // SERVER_H
