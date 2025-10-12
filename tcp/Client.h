#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <sstream>
#include "SocketHandler.h"

class Client : public SocketHandler
{
public:
    static constexpr int PORT = 5555;
    static constexpr auto CLIENT_NAME = "Client Lalala";

    bool ConnectToServer();
    void Run();
};

#endif // CLIENT_H
