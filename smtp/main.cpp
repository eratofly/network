#include "SmtpClient.h"
#include <iostream>
#include <stdexcept>

const std::string SERVER_IP = "127.0.0.1";
constexpr int SERVER_PORT = 2525;
const std::string SENDER_EMAIL = "alice@mail.test";
const std::string RECIPIENT_EMAIL = "bob@mail.test";

int main() {
    std::cout << "Start" << std::endl;

    try {
        SmtpClient client(SERVER_IP, SERVER_PORT);

        client.Connect();

        client.SendMail(
            SENDER_EMAIL,
            RECIPIENT_EMAIL,
            "Test sending email",
            "Hello everybody! Happy New Yaer!!!"
        );

    } catch (const std::exception& e) {
        std::cerr << "[CRITICAL ERROR]: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "End" << std::endl;
    return 0;
}