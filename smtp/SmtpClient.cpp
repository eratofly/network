#include "SmtpClient.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

SmtpClient::SmtpClient(const std::string &serverIp, const int port) :
    m_socket(-1), m_serverIp(serverIp), m_port(port), m_isConnected(false)
{
}

SmtpClient::~SmtpClient() { Disconnect(); }

void SmtpClient::Connect()
{
    if (m_isConnected)
        return;

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0)
    {
        throw std::runtime_error("Error creating socket");
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);

    if (inet_pton(AF_INET, m_serverIp.c_str(), &serverAddr.sin_addr) <= 0)
    {
        close(m_socket);
        throw std::runtime_error("Invalid IP address");
    }

    std::cout << "Connection to " << m_serverIp << ":" << m_port << "..." << std::endl;

    if (connect(m_socket, (sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    {
        close(m_socket);
        throw std::runtime_error("No such connection");
    }

    m_isConnected = true;

    std::string response;
    ReadResponse(response);
    CheckResponse(response, 220);
}

void SmtpClient::Disconnect()
{
    if (m_isConnected && m_socket >= 0)
    {
        close(m_socket);
        m_socket = -1;
        m_isConnected = false;
        std::cout << "Connection is end" << std::endl;
    }
}

void SmtpClient::ReadResponse(std::string &responseBuffer)
{
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));

    ssize_t bytesRead = read(m_socket, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0)
    {
        throw std::runtime_error("Error reading from socket");
    }

    responseBuffer = std::string(buffer);
    std::cout << "S: " << responseBuffer;
    if (responseBuffer.back() != '\n')
        std::cout << std::endl;
}

void SmtpClient::SendRaw(const std::string &data)
{
    std::cout << "C: " << data;
    if (write(m_socket, data.c_str(), data.length()) < 0)
    {
        throw std::runtime_error("Error writing to socket");
    }
}

void SmtpClient::CheckResponse(const std::string &response, int expectedCode)
{
    std::string codeStr = std::to_string(expectedCode);
    if (response.substr(0, 3) != codeStr)
    {
        std::stringstream ss;
        ss << "Error SMTP. Waiting code " << expectedCode << ", receive response: " << response;
        throw std::runtime_error(ss.str());
    }
}

void SmtpClient::SendCommand(const std::string &command, const std::string &argument, int expectedCode)
{
    std::stringstream ss;
    ss << command;
    if (!argument.empty())
    {
        ss << " " << argument;
    }
    ss << "\r\n";

    SendRaw(ss.str());

    std::string response;
    ReadResponse(response);
    CheckResponse(response, expectedCode);
}

void SmtpClient::SendMail(const std::string &sender, const std::string &recipient, const std::string &subject,
                          const std::string &body)
{
    SendCommand("HELO", "smtp", 250);

    std::string fromArg = "<" + sender + ">";
    SendCommand("MAIL FROM:", fromArg, 250);

    std::string toArg = "<" + recipient + ">";
    SendCommand("RCPT TO:", toArg, 250);

    SendCommand("DATA", "", 354);

    std::stringstream emailContent;
    emailContent << "From: " << sender << "\r\n";
    emailContent << "To: " << recipient << "\r\n";
    emailContent << "Subject: " << subject << "\r\n";
    emailContent << "\r\n";
    emailContent << body << "\r\n";
    emailContent << ".\r\n";

    SendRaw(emailContent.str());

    std::string response;
    ReadResponse(response);
    CheckResponse(response, 250);

    SendCommand("QUIT", "", 221);
}
