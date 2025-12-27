#include <iostream>
#include <cstring>
#include "RdtSender.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: ./rdt_sender <ip> <port> <file> [-d]" << std::endl;
        return 1;
    }

    std::string ip = argv[1];
    int port = std::atoi(argv[2]);
    std::string filename = argv[3];
    bool debug = (argc > 4 && std::string(argv[4]) == "-d");

    RdtSender sender(ip, port, debug);
    sender.SendFile(filename);

    return 0;
}