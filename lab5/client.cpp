#include <iostream>
#include "socket.h"

# define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 5544

int main() {
    auto clientSock = Socket(5555);
    clientSock.writeToServer(SERVER_ADDRESS, SERVER_PORT, "Hello!");
    // block
    std::string receivedMessage = clientSock.getResponse();

    std::cout << receivedMessage << std::endl;
    return 0;
}