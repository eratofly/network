#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <sys/stat.h>
#include <functional>

#define BUFFER_SIZE 4096
#define PORT 8888

using namespace std;

string GetCacheFilename(const string& url) {
    hash<string> hasher;
    size_t hashVal = hasher(url);
    return "cache/" + to_string(hashVal);
}

bool IsCached(const string& fileName) {
    struct stat buffer;
    return (stat(fileName.c_str(), &buffer) == 0);
}

void ParseUrl(string url, string &host, string &path, int &port) {
    port = 80;
    size_t httpPos = url.find("http://");
    if (httpPos == 0) url = url.substr(7);

    size_t pathPos = url.find('/');
    if (pathPos != string::npos) {
        host = url.substr(0, pathPos);
        path = url.substr(pathPos);
    } else {
        host = url;
        path = "/";
    }

    size_t portPos = host.find(':');
    if (portPos != string::npos) {
        port = stoi(host.substr(portPos + 1));
        host = host.substr(0, portPos);
    }
}

void SendFromCache(int clientSocket, const string& fileName) {
    cout << "[CACHE HIT] Serving from: " << fileName << endl;
    ifstream cacheFile(fileName, ios::binary);
    char buffer[BUFFER_SIZE];

    while (cacheFile.read(buffer, BUFFER_SIZE) || cacheFile.gcount() > 0) {
        send(clientSocket, buffer, cacheFile.gcount(), 0);
    }
    cacheFile.close();
}

void HandleRemoteRequest(int clientSocket, string host, string path, int port, string cacheFileName) {
    cout << "[CACHE MISS] Fetching from: " << host << path << endl;

    struct hostent* server = gethostbyname(host.c_str());
    if (server == NULL) {
        cerr << "Error: No such host " << host << endl;
        return;
    }

    int remoteSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr;
    bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&serverAddr.sin_addr.s_addr, server->h_length);
    serverAddr.sin_port = htons(port);

    if (connect(remoteSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error connecting to remote server" << endl;
        close(remoteSocket);
        return;
    }

    string request = "GET " + path + " HTTP/1.0\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
    send(remoteSocket, request.c_str(), request.length(), 0);

    ofstream cacheFile(cacheFileName, ios::binary);
    char buffer[BUFFER_SIZE];
    int bytesRead;

    while ((bytesRead = recv(remoteSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        send(clientSocket, buffer, bytesRead, 0);
        cacheFile.write(buffer, bytesRead);
    }

    cacheFile.close();
    close(remoteSocket);
}

void HandleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);

    if (bytes <= 0) {
        close(clientSocket);
        return;
    }

    string request(buffer, bytes);
    stringstream ss(request);
    string method, url, protocol;
    ss >> method >> url >> protocol;

    if (method == "GET") {
        string host, path;
        int port;
        ParseUrl(url, host, path, port);

        if (port == 443) {
            cout << "HTTPS not supported in basic lab proxy." << endl;
            close(clientSocket);
            return;
        }

        string fileName = GetCacheFilename(url);

        if (IsCached(fileName)) {
            SendFromCache(clientSocket, fileName);
        } else {
            HandleRemoteRequest(clientSocket, host, path, port, fileName);
        }
    }

    close(clientSocket);
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    system("mkdir -p cache");

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Error opening socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error on binding");
        exit(1);
    }

    listen(serverSocket, 5);
    cout << "Proxy Server started on port " << PORT << endl;
    cout << "Cache directory: ./cache/" << endl;

    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            perror("Error on accept");
            continue;
        }

        thread clientThread(HandleClient, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}