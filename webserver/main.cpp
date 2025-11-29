#include <iostream>
#include <signal.h>
#include "Webserver.h"

void SetupSignalHandler(Webserver *serverInstance)
{
    struct sigaction sa{};
    sa.sa_handler = Webserver::SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1)
    {
        std::cerr << "Error: Cannot set SIGINT handler." << std::endl;
    }
}

int main()
{
    const int port = 8080;
    const std::string ipAddress = "0.0.0.0";

    Webserver server(ipAddress, port);
    Webserver::SetInstance(&server);
    SetupSignalHandler(&server);
    server.Start();

    return 0;
}
