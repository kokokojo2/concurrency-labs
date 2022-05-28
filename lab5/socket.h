#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#include "Logger.h"

#ifndef PC_LAB1_SOCKET_H
#define PC_LAB1_SOCKET_H


class Socket : Logger {
    int socketDesc;
    int port;
    int connectionDesc{};
public:
    explicit Socket(int port);
    void binAndListen();
    std::string waitForMessage();
    void reply(const std::string& message);
    void writeToServer(const std::string& serverIP, int serverPort, const std::string& message);
    std::string getResponse();
};


#endif //PC_LAB1_SOCKET_H
