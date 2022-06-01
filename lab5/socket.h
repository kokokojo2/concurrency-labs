#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "Logger.h"

#ifndef PC_LAB1_SOCKET_H
#define PC_LAB1_SOCKET_H


class Socket : public Logger {
protected:
    int socketDesc;
    int port;
public:
    explicit Socket(int port);
};

class ClientSocket : Socket {
public:
    explicit ClientSocket(int port) : Socket(port) {};
    void connect(const std::string& serverIP, int serverPort);
    void writeMessage(const std::string &message);
    std::string readMessage();
};

class ServerSocket : Socket {
public:
    explicit ServerSocket(int port) : Socket(port) {};
    void binAndListen();
    int waitForConnection();
    std::string getMessage(int connectionId);
    void sendMessage(int connectionId, const std::string& message);
};

#endif //PC_LAB1_SOCKET_H
