#include "Webserver.h"
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>

const int bufSize = 30720;

std::mutex Webserver::m_consoleMutex;
Webserver *Webserver::m_instance = nullptr;

Webserver::Webserver(std::string ipAddress, int port) :
    m_ipAddress(std::move(ipAddress)), m_port(port), m_serverSocket(-1), m_isStopping(false)
{
    m_socketAddressLen = sizeof(m_socketAddress);
}

Webserver::~Webserver() { CloseServer(); }

void Webserver::SetInstance(Webserver *instance) { m_instance = instance; }

void Webserver::SignalHandler(int signal)
{
    if (signal == SIGINT && m_instance != nullptr)
    {
        m_instance->Log("SIGINT received, shutting down server...");
        m_instance->m_isStopping = true;

        if (m_instance->m_serverSocket != -1)
        {
            shutdown(m_instance->m_serverSocket, SHUT_RDWR);
        }
    }
}

int Webserver::StartServer()
{
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket < 0)
    {
        Log("Error: Cannot create socket");
        return -1;
    }

    m_socketAddress.sin_family = AF_INET;
    m_socketAddress.sin_port = htons(m_port);
    m_socketAddress.sin_addr.s_addr = inet_addr(m_ipAddress.c_str());

    if (bind(m_serverSocket, (struct sockaddr *) &m_socketAddress, m_socketAddressLen) < 0)
    {
        Log("Error: Cannot bind socket to address");
        return -1;
    }

    if (listen(m_serverSocket, 20) < 0)
    {
        Log("Error: Socket listen failed");
        return -1;
    }

    std::ostringstream ss;
    ss << "\n*** Server started on " << m_ipAddress << ":" << m_port << " ***\n";
    Log(ss.str());

    return 0;
}

void Webserver::CloseServer()
{
    if (m_serverSocket != -1)
    {
        close(m_serverSocket);
        m_serverSocket = -1;
    }
}

void Webserver::Start()
{
    if (StartServer() != 0)
    {
        return;
    }

    while (!m_isStopping)
    {
        int clientSocket = accept(m_serverSocket, reinterpret_cast<struct sockaddr *>(&m_socketAddress), &m_socketAddressLen);

        if (m_isStopping)
        {
            break;
        }

        if (clientSocket < 0)
        {
            Log("Error: Accept failed or interrupted");
            continue;
        }

        // ИСПРАВЛЕНИЕ: Вызов std::thread теперь корректен, так как HandleClient нестатический
        std::thread clientThread(&Webserver::HandleClient, this, clientSocket);
        clientThread.detach();
    }

    Log("*** Server gracefully stopped. ***");
}


void Webserver::HandleClient(int clientSocket)
{
    char buffer[bufSize] = {0};

    int bytesRead = read(clientSocket, buffer, bufSize);
    if (bytesRead <= 0)
    {
        close(clientSocket);
        return;
    }

    std::string request(buffer);

    std::string fileName = ParseRequest(request);

    {
        std::ostringstream ss;
        ss << "Handling request for: " << fileName << " (Thread ID: " << std::this_thread::get_id() << ")";
        Log(ss.str());
    }

    std::ifstream file(fileName);

    if (file.good())
    {
        std::stringstream fileBuffer;
        fileBuffer << file.rdbuf();
        std::string content = fileBuffer.str();
        std::string contentType = GetContentType(fileName);

        SendResponse(clientSocket, content, contentType, 200);
    } else
    {
        std::string content = "<html><body><h1>404 Not Found</h1><p>The requested file does not exist.</p></body></html>";
        SendResponse(clientSocket, content, "text/html", 404);
    }

    close(clientSocket);
}
std::string Webserver::ParseRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string method, path, protocol;
    iss >> method >> path >> protocol;

    if (path == "/")
    {
        return "index.html";
    }

    if (!path.empty() && path[0] == '/')
    {
        return path.substr(1);
    }

    return path;
}

std::string Webserver::GetContentType(const std::string &filePath)
{
    if (filePath.find(".html") != std::string::npos)
        return "text/html";
    if (filePath.find(".css") != std::string::npos)
        return "text/css";
    if (filePath.find(".js") != std::string::npos)
        return "application/javascript";
    if (filePath.find(".jpg") != std::string::npos || filePath.find(".jpeg") != std::string::npos)
        return "image/jpeg";
    if (filePath.find(".png") != std::string::npos)
        return "image/png";
    return "text/plain";
}

void Webserver::SendResponse(int clientSocket, const std::string &content, const std::string &contentType,
                             int statusCode)
{
    std::string statusMsg = (statusCode == 200) ? "200 OK" : "404 Not Found";

    std::ostringstream response;
    response << "HTTP/1.1 " << statusMsg << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << content;

    std::string responseStr = response.str();
    write(clientSocket, responseStr.c_str(), responseStr.size());
}

void Webserver::Log(const std::string &message)
{
    std::lock_guard<std::mutex> lock(m_consoleMutex);
    std::cout << "[Server] " << message << std::endl;
}