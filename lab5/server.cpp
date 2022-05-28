#include <iostream>
#include "socket.h"

int main() {
    auto serverSock = Socket(5544);
    serverSock.binAndListen();
    // block
    std::string receivedMessage = serverSock.waitForMessage();

    std::cout << receivedMessage << std::endl;
    serverSock.reply("World!");
    return 0;
}