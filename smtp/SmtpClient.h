#ifndef SMTP_CLIENT_HPP
#define SMTP_CLIENT_HPP

#include <string>

class SmtpClient {
public:
    SmtpClient(const std::string& server, int m_port);
    ~SmtpClient();

    bool ConnectToServer();

    bool SendEmail(const std::string& sender,
                   const std::string& recipient,
                   const std::string& subject,
                   const std::string& body);

private:
    std::string m_serverHost;
    int m_port;
    int m_clientSocket;
    bool m_isConnected;
    std::string GetIpByHostname(const std::string& hostname);
    std::string ReadLine();
    void SendCommand(const std::string& command);
    bool ExpectResponse(int expectedCode);
};

#endif // SMTP_CLIENT_HPP