#include <cstdlib>
#include <ctime>
#include <iostream>
#include "DnsResolver.h"

int main(int argc, char *argv[])
{
    srand(time(0));

    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <domain> <type> [-d]" << std::endl;
        std::cout << "Example: " << argv[0] << " example.com A -d" << std::endl;
        return 1;
    }

    std::string domain = argv[1];
    std::string type = argv[2];
    bool debug = false;

    if (argc > 3 && std::string(argv[3]) == "-d")
    {
        debug = true;
    }

    DnsResolver resolver(debug);
    resolver.Resolve(domain, type);

    return 0;
}
