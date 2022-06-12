#include <iostream>
#include <thread>

#include "socket.h"
#include "constants.h"
#include "protocol.cpp"

class GameClient : public Logger {
private:
    ClientSocket socket;

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


    TicTacToeCommand inputCoordsDialog(TicTacToeCommand playTurnCommand, std::vector<std::vector<int>> boardState) {
        int xCoord, yCoord;

        while (true) {
            std::cout << "Enter x coord: ";
            std::cin >> xCoord;
            std::cout << "Enter y coord: ";
            std::cin >> yCoord;

            if (xCoord >= BOARD_SIZE || yCoord >= BOARD_SIZE) {
                std::cout << "One of the coords is out of range" << std::endl;
                continue;
            }
            // checking if the cell is empty
            if(boardState[xCoord][yCoord] != 0) {
                std::cout << "This cell is already marked with " + getCellRepr(boardState[xCoord][yCoord])
                             + ". Please, try again." << std::endl;
                continue;
            }

            playTurnCommand.xCoord = xCoord;
            playTurnCommand.yCoord = yCoord;
            return playTurnCommand;
        }

    }

    user connectToGame() {
        TicTacToeCommand connectToGameCommand;
        connectToGameCommand.command = command.connectToGame;
        writeCommand(this->socket, connectToGameCommand);
        TicTacToeMessage responseMessage = waitForValidMessage(this->socket);
        user currentUser{};

         // user is connected to game as a player
        if (responseMessage.status == status.connectedAsPlayer) {
            currentUser.type = user_type.player;
            currentUser.id = responseMessage.userId;
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

    void playTurn(user player, TicTacToeMessage currentBoardMessage) {
        this->print("It is your turn now.", false);
        printBoard(currentBoardMessage.boardState);

        this->print("Enter coords of the cell to put" + getCellRepr(player.id) + " mark there.", false);
        TicTacToeCommand playTurnCommand;
        playTurnCommand.command = command.playTurn;

        playTurnCommand = inputCoordsDialog(playTurnCommand, currentBoardMessage.boardState);
        writeCommand(this->socket, playTurnCommand);
        waitForValidMessage(this->socket, status.accepted);
    }

    void playGame(user player) {
        auto gameStartedMessage = waitForValidMessage(this->socket, status.gameStarted);
        this->print("The second player is connected and the game is starting.", false);

        while (true) {
            this->print("Please, wait until your turn starts.", false);
            auto nextActionMessage = waitForValidMessage(this->socket);

            if (nextActionMessage.status == status.gameFinished) {
                printBoard(nextActionMessage.boardState);
                if (nextActionMessage.winnerId == player.id)
                    this->print("You won the game!", false);
                else if (nextActionMessage.winnerId == -1)
                    this->print("Well done, the game ended as tie.", false);
                else
                    this->print("You lost the game :(", false);
                break;
            }

            this->playTurn(player, nextActionMessage);
            this->print("Your turn has been ended.", false);
        }
    }

    void spectateGame(user spectator) {
        while (true) {
            auto gameProgressMessage = waitForValidMessage(this->socket);
            printBoard(gameProgressMessage.boardState);
            if (gameProgressMessage.status == status.gameProgress) {
                this->print("User" + std::to_string(gameProgressMessage.userId)
                            + " makes his turn. (sign - " + getCellRepr(gameProgressMessage.userId) + ")", false);
            }
            if (gameProgressMessage.status == status.gameFinished) {
                this->print("Game was finished!", false);
                if (gameProgressMessage.winnerId != -1)
                    this->print("User" + std::to_string(gameProgressMessage.winnerId) + " has won the game", false);
                if (gameProgressMessage.winnerId == -1) {
                    this->print("It's tie!", false);
                }
                break;
            }
        }
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
            this->playGame(currentUser);
        }
        if (currentUser.type == user_type.spectator) {
            this->spectateGame(currentUser);
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