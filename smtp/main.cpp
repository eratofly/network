#include <iostream>
#include "SmtpClient.h"

using namespace std;

int main() {
    string smtpServer = "127.0.0.1";
    int m_port = 1025;

    string sender = "alice@mail.com";
    string recipient = "bob@mail.com";
    string subject = "Congratulations!";
    string body = "Hello!\nHappy New Year!";

    SmtpClient client(smtpServer, m_port);

    if (client.ConnectToServer()) {
        cout << "Connected successfully." << endl;

        if (client.SendEmail(sender, recipient, subject, body)) {
            cout << "Email sent successfully!" << endl;
        } else {
            cerr << "Failed to send email." << endl;
            return 1;
        }
    } else {
        cerr << "Failed to connect to server." << endl;
        return 1;
    }

    return 0;
}