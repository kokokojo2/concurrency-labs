#include <iostream>
#include <thread>
#include <mutex>
#include "protocol.cpp"
#include "socket.h"
#include "constants.h"

std::mutex resourcesMutex;

// TODO: one game, two clients
class GameServer : public Logger {
private:
    ServerSocket socket;
    int playerOneConnectionDesc = -1;
    int playerTwoConnectionDesc = -1;

    void setPlayerOneConnection(int connectionDesc) {
        this->playerOneConnectionDesc = connectionDesc;
    }

    void setPlayerTwoConnection(int connectionDesc) {
        this->playerTwoConnectionDesc = connectionDesc;
    }

    int setPlayer(int connectionDesc) {
        std::lock_guard<std::mutex> lock(resourcesMutex);
        if (this->playerOneConnectionDesc == -1) {
            this->setPlayerOneConnection(connectionDesc);
            return 1;
        }
        if (this->playerTwoConnectionDesc == -1) {
            this->setPlayerTwoConnection(connectionDesc);
            return 2;
        }
        return -1;
    }

    std::string waitForMessage(int connectionDesc) {
        while (true) {
            std::string message = this->socket.getMessage(connectionDesc);
            if (!message.empty()) return message;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    TicTacToeCommand waitForValidCommand(int connectionDesc) {
        while (true) {
            std::string message = this->waitForMessage(connectionDesc);
            TicTacToeCommand parsedCommand = parseRequest(message);
            if (parsedCommand.valid) return parsedCommand;

            this->print("Got invalid command. This is likely to be the problem with client.", false);
            printCommand(parsedCommand);

            TicTacToeMessage messageInvalidCommand;
            messageInvalidCommand.status = status.previousCommandInvalid;
            this->sendMessage(connectionDesc, messageInvalidCommand);
        }
    }

    void sendMessage(int connectionDesc, const TicTacToeMessage&  message) {
        socket.sendMessage(connectionDesc, messageToString(message));
    }

    user handleNewUser(int connectionDesc) {
        auto parsedCommand = this->waitForValidCommand(connectionDesc);
        this->print(
                "New client has connected and wants to " + getReprCommand(parsedCommand.command) + ".",
                true);

        user currentUser{};
        currentUser.type = user_type.noType;
        TicTacToeMessage message;

        if (parsedCommand.command == command.watchGame) {
            currentUser.type = user_type.spectator;
            message.status = status.accepted;
        }
        if (parsedCommand.command == command.disconnect) {
            currentUser.type = user_type.exited;
            message.status = status.accepted;
        }
        if (parsedCommand.command == command.connectToGame) {
            // trying to set user if slots are not busy
            currentUser.id = setPlayer(connectionDesc);
            // returning message with error if there are no free slot
            if (currentUser.id == -1) {
                message.status = status.error;
                this->print("There is no free slot for this user.", true);
            } else {
                // user was set successfully
                currentUser.type = user_type.player;
                message.status = status.accepted;
            }
        }

        this->sendMessage(connectionDesc, message);
        return currentUser;
    }

    void serveClient(int connectionDesc) {
        user currentUser{};
        do {
            currentUser = this->handleNewUser(connectionDesc);
        } while (currentUser.type == user_type.noType);
        this->print("New user was logged in: " + getUserRepr(currentUser), true);

        if (currentUser.type == user_type.exited) return;
        if (currentUser.type == user_type.player) {
            // TODO: start game, handle player
        }
        if (currentUser.type == user_type.spectator) {
            // TODO: handle spectator
        }
    }
public:
    GameServer() : socket(SERVER_PORT) {
        this->prefix = "Server";
    };
    void run() {
        socket.binAndListen();
        while (true) {
            int connectionDesc = socket.waitForConnection();
            std::thread t1(&GameServer::serveClient, this, connectionDesc);
            t1.detach();
            // TODO: create procedure thread that works with client until it disconnect
        }
    };
};

// TODO: process every connection in different thread
int main() {
    auto server = GameServer();
    server.run();
    return 0;
}