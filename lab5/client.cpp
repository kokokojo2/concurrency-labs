#include <iostream>
#include <chrono>
#include <thread>
#include "socket.h"
#include "constants.h"
#include "protocol.cpp"

class GameClient : public Logger {
private:
    ClientSocket socket;

    // TODO: make chooseCommandDialog instead
    int optionDialog(std::vector<std::string> options) {
        this->print("Showing choose option dialog:", false);
        int option;
        while (true) {
            std::cout << "Choose one of the following options:" << std::endl;
            for(int i = 0; i < options.size(); i++) {
                std::cout << std::to_string(i) << ") " << options[i] << std::endl;
            }

            std::cin >> option;
            if (option < options.size()) return option;
        }
    }

    user connectToGame() {
        TicTacToeCommand connectToGameCommand;
        connectToGameCommand.command = command.connectToGame;
        writeCommand(this->socket, connectToGameCommand);
        TicTacToeMessage responseMessage = waitForValidMessage(this->socket);
        user currentUser{};

         // user is connected to game as a player
        if (responseMessage.status == status.accepted) {
            currentUser.type = user_type.player;
         // the server is busy
        } else {
            this->print("The server has no free slots", false);
            // choose between watch game or leave
            std::vector<std::string> options{"watch current game.", "disconnect."};
            // returns the index of chosen option
            int option = this->optionDialog(options);

            TicTacToeCommand optionCommand;
            optionCommand.command = option == 0 ? command.watchGame : command.disconnect;

            writeCommand(this->socket, optionCommand);
            auto response = waitForValidMessage(this->socket);
            if (response.status == status.accepted) {
                currentUser.type = optionCommand.command == command.watchGame ? user_type.spectator : user_type.exited;
            }
        }

        return currentUser;
    }
public:
    explicit GameClient(int port) : socket(port) {
        this->prefix = "Client";

    };
    void run() {
        socket.connect(SERVER_ADDRESS, SERVER_PORT);
        user currentUser = this->connectToGame();
        if (currentUser.type == user_type.exited) return;

        this->print("You have successfully logged in: " + getUserRepr(currentUser), false);
        if (currentUser.type == user_type.player) {
            // TODO: start game, handle player
        }
        if (currentUser.type == user_type.spectator) {
            // TODO: handle spectator
        }
    };
};
int main() {
    std::cout << "Enter the port which the client should use: ";
    int port;
    std::cin >> port;
    auto client = GameClient(port);
    client.run();
    return 0;
}