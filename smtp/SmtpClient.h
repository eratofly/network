#ifndef SMTP_CLIENT_HPP
#define SMTP_CLIENT_HPP

#include <string>

class SmtpClient {
public:
    SmtpClient(const std::string& serverIp, int port);
    ~SmtpClient();

    void Connect();
    void SendCommand(const std::string& command, const std::string& argument, int expectedCode);
    void SendMail(const std::string& sender, const std::string& recipient, const std::string& subject, const std::string& body);
    void Disconnect();

private:
    int m_socket;
    std::string m_serverIp;
    int m_port;
    bool m_isConnected;

    void ReadResponse(std::string& responseBuffer);
    void SendRaw(const std::string& data);
    void CheckResponse(const std::string& response, int expectedCode);
};

#endif // SMTP_CLIENT_HPP