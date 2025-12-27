#include <iostream>
#include <cstring>
#include "RdtReceiver.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./rdt_receiver <port> <file> [-d]" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    std::string filename = argv[2];
    bool debug = (argc > 3 && std::string(argv[3]) == "-d");

    RdtReceiver receiver(port, debug);
    receiver.ReceiveFile(filename);

    return 0;
}