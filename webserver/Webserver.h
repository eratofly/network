#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <netinet/in.h>
#include <mutex>
#include <atomic>

class Webserver {
public:
    Webserver(std::string  ipAddress, int port);
    ~Webserver();

    void Start();
    static void SignalHandler(int signal);
    static void SetInstance(Webserver* instance);

private:
    std::string m_ipAddress;
    int m_port;
    int m_serverSocket;
    struct sockaddr_in m_socketAddress{};
    unsigned int m_socketAddressLen;
    std::atomic<bool> m_isStopping;
    static Webserver* m_instance;
    static std::mutex m_consoleMutex;
    int StartServer();
    void CloseServer();
    void HandleClient(int clientSocket);
    std::string ParseRequest(const std::string& request);
    std::string GetContentType(const std::string& filePath);
    void SendResponse(int clientSocket, const std::string &content, const std::string &contentType,
                             int statusCode);
    void Log(const std::string& message);
};

#endif