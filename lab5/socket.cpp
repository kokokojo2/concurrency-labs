//
// Created by kokokojo2 on 28.05.22.
//

#include "socket.h"
#define MAX_QUEUE_SIZE 25

Socket::Socket(int port) {
    this->prefix = "in Socket";
    this->port = port;

    this->socketDesc = socket(AF_INET, SOCK_STREAM, 0);
    if (this->socketDesc == -1) {
        this->print("Some error occur while initializing.", false);
        exit(EXIT_FAILURE);
    }
    this->print("A TCP socket was created.", false);
}

void Socket::binAndListen() {
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(this->port);

    if (bind(this->socketDesc, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        this->print("Some error occur while binding socket.", false);
        exit(EXIT_FAILURE);
    }

    if(listen(this->socketDesc, MAX_QUEUE_SIZE) != 0) {
        this->print("Failed to listen...", false);
        exit(EXIT_FAILURE);
    }
    this->print("The socket was bound and ready for communication.", false);
}

std::string Socket::waitForMessage() {
    struct sockaddr_in clientAddr{};
    socklen_t clientAddressLen = sizeof(clientAddr);

    this->connectionDesc = accept(
            this->socketDesc,
            (struct sockaddr*)&clientAddr,
            &clientAddressLen
    );
    if(this->connectionDesc == -1) {
        this->print("Some error occur while accepting connection.", true);
        exit(EXIT_FAILURE);
    }

    int messageMaxLength = 1024;
    char message[messageMaxLength];

    long messageSize = read(this->connectionDesc, message, messageMaxLength);
    this->print("Received message of size = " + std::to_string(messageSize) + " bytes.", true);
    if (messageSize == -1) {
        this->print("Some error occur while receiving the message.", true);
        exit(EXIT_FAILURE);
    }
    message[messageSize] = '\0';
    return std::string(message);
}

void Socket::reply(const std::string& message) {
    if (write(this->connectionDesc, message.c_str(), sizeof(message.c_str())) == -1) {
        this->print("Some error occur while replying to the message.", true);
        exit(EXIT_FAILURE);
    }
}

void Socket::writeToServer(const std::string& serverIP, int serverPort, const std::string &message) {
    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    if (inet_aton(serverIP.c_str() , &serverAddress.sin_addr) == 0) {
        exit(EXIT_FAILURE);
    }

    if (connect(
            this->socketDesc,
            (struct sockaddr *)&serverAddress,
            sizeof(serverAddress)
    ) == -1) {
        this->print("Some error occur while connecting to the server.", false);
        exit(EXIT_FAILURE);
    }

    write(this->socketDesc, message.c_str(), sizeof(message.c_str()));
    this->print("Message was sent.", false);
}

std::string Socket::getResponse() {
    int bufferSize = 1024;
    char receivedMessage [bufferSize];

    int messageSize = read(
            this->socketDesc,
            receivedMessage,
            bufferSize
    );

    if (messageSize == -1) {
        this->print("Some error occur while receiving the message.", false);
        exit(EXIT_FAILURE);
    }
    return std::string(receivedMessage);
}
