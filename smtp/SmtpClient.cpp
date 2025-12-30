#include "SmtpClient.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

using namespace std;

SmtpClient::SmtpClient(const string& server, int m_port)
    : m_serverHost(server), m_port(m_port), m_clientSocket(-1), m_isConnected(false) {}

SmtpClient::~SmtpClient() {
    if (m_isConnected && m_clientSocket != -1) {
        close(m_clientSocket);
    }
}

string SmtpClient::GetIpByHostname(const string& hostname) {
    struct hostent* he;
    struct in_addr** addrList;
    he = gethostbyname(hostname.c_str());
    if (he == NULL) return "";
    addrList = (struct in_addr**)he->h_addr_list;
    return inet_ntoa(*addrList[0]);
}

string SmtpClient::ReadLine() {
    string line = "";
    char c;
    while (read(m_clientSocket, &c, 1) > 0) {
        line += c;
        if (line.length() >= 2 && line.substr(line.length() - 2) == "\r\n") {
            break;
        }
    }
    return line;
}

void SmtpClient::SendCommand(const string& command) {
    cout << "C: " << command;
    write(m_clientSocket, command.c_str(), command.length());
}

bool SmtpClient::ExpectResponse(int expectedCode) {
    string fullResponse = "";
    string lastLine = "";

    while (true) {
        lastLine = ReadLine();
        fullResponse += lastLine;
        if (lastLine.length() < 4) break;
        if (lastLine[3] == ' ') break;
    }

    cout << "S: " << fullResponse;

    if (fullResponse.empty()) return false;

    try {
        int actualCode = stoi(fullResponse.substr(0, 3));
        return actualCode == expectedCode;
    } catch (...) {
        return false;
    }
}

bool SmtpClient::ConnectToServer() {
    m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_clientSocket < 0) {
        cerr << "Error creating socket" << endl;
        return false;
    }

    string serverIp = GetIpByHostname(m_serverHost);
    if (serverIp.empty()) {
        cerr << "Could not resolve hostname" << endl;
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_port);
    inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);

    if (connect(m_clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection failed" << endl;
        return false;
    }

    if (!ExpectResponse(220)) {
        close(m_clientSocket);
        return false;
    }

    m_isConnected = true;
    return true;
}

bool SmtpClient::SendEmail(const string& sender, const string& recipient, const string& subject, const string& body) {
    if (!m_isConnected) {
        cerr << "Not connected to server" << endl;
        return false;
    }

    SendCommand("EHLO myclient.com\r\n");
    if (!ExpectResponse(250)) return false;

    SendCommand("MAIL FROM: <" + sender + ">\r\n");
    if (!ExpectResponse(250)) return false;

    SendCommand("RCPT TO: <" + recipient + ">\r\n");
    if (!ExpectResponse(250)) return false;

    SendCommand("DATA\r\n");
    if (!ExpectResponse(354)) return false;

    string fullMessage = "Subject: " + subject + "\r\n\r\n" + body + "\r\n.\r\n";
    SendCommand(fullMessage);
    if (!ExpectResponse(250)) return false;

    SendCommand("QUIT\r\n");
    ExpectResponse(221);

    close(m_clientSocket);
    m_isConnected = false;
    m_clientSocket = -1;

    return true;
}