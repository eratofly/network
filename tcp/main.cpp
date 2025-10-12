#include <iostream>
#include "Client.h"
#include "Server.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " [client|server]" << std::endl;
        return 1;
    }

    if (std::string(argv[1]) == "client")
    {
        Client client;
        client.Run();
    } else if (std::string(argv[1]) == "server")
    {
        Server server;
        server.Run();
    } else
    {
        std::cerr << "Invalid argument. Use 'client' or 'server'" << std::endl;
        return 1;
    }

    return 0;
}
