#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "protocol.cpp"
#include "socket.h"
#include "constants.h"

std::mutex resourcesMutex;
std::mutex playerOneTurnStartMutex, playerTwoTurnStartMutex;
std::mutex playerOneFinishedPlayingTurnMutex, playerTwoFinishedPlayingTurnMutex;


std::condition_variable gameStartCV, playerOneCanPlayTurnCV, playerTwoCanPlayTurnCV;
std::condition_variable playerOneFinishedPlayingTurnCV, playerTwoFinishedPlayingTurnCV;

// TODO: one game, two clients
class GameServer : public Logger {
private:
    // communication data
    ServerSocket socket;
    int playerOneConnectionDesc = -1;
    int playerTwoConnectionDesc = -1;

    // game data
    const static int boardSize = 3;
    // 0 - cell is empty
    // 1 - user with id=1 owns the cell
    // 2 - user with id=1 owns the cell
    int board[boardSize][boardSize] = {0};

    // board methods
    void clearBoard() {
        for (int i = 0; i < boardSize; i++) {
            for (int j = 0; j < boardSize; j++) {
                this->board[i][j] = 0;
            }
        }
    }

    // player methods
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

    user handleNewUser(int connectionDesc) {
        auto parsedCommand = waitForValidCommand(this->socket, connectionDesc);
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

        sendMessage(this->socket, connectionDesc, message);
        currentUser.connectionDesc = connectionDesc;
        return currentUser;
    }

    bool canGameStart() const {
        return this->playerTwoConnectionDesc != -1 && this->playerOneConnectionDesc != -1;
    }

    void waitForGameStart() {
        while (true) {
            if (this->canGameStart()) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void waitForTurnStart(user player) const {
        if (player.connectionDesc == this->playerOneConnectionDesc) {
            std::unique_lock<std::mutex> lock(playerOneTurnStartMutex);
            playerOneCanPlayTurnCV.wait(lock);
        }
        if (player.connectionDesc == this->playerTwoConnectionDesc) {
            std::unique_lock<std::mutex> lock(playerTwoTurnStartMutex);
            playerTwoCanPlayTurnCV.wait(lock);
        }
    }

    void notifyTurnEnd(user player) const {
        if(player.connectionDesc == this->playerOneConnectionDesc) {
            playerOneFinishedPlayingTurnCV.notify_all();
        }
        if(player.connectionDesc == this->playerTwoConnectionDesc) {
            playerTwoFinishedPlayingTurnCV.notify_all();
        }
    }

    void gameScheduler() {
        waitForGameStart();
        // sleeping here to avoid notifying the condition_variable
        // before serveClient actually starts waiting for it
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        this->clearBoard();
        this->print("Game scheduler sending signal to start the game!", true);

        // notifying serveClient that game is started
        gameStartCV.notify_all();
        this->playUntilWin();
    }

    // TODO: switch to connectionDesc instead of player id
    void playTurnAndCheck(int playerId) {
        this->print("Game scheduler sending signal to thread " + std::to_string(playerId) + "  to start turn", true);
        if (playerId == 1) {
            playerOneCanPlayTurnCV.notify_all();
            this->print("sent", false);
            std::unique_lock<std::mutex> lock(playerOneFinishedPlayingTurnMutex);
            playerOneFinishedPlayingTurnCV.wait(lock);
        }
        if (playerId == 2) {
            playerTwoCanPlayTurnCV.notify_all();
            this->print("sent", false);
            std::unique_lock<std::mutex> lock(playerTwoFinishedPlayingTurnMutex);
            playerTwoFinishedPlayingTurnCV.wait(lock);
        }
        // TODO: check result
    }

    void playUntilWin() {
        // TODO: get result if win - break the loop and set winner
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            // player one makes his turn
            playTurnAndCheck(1);
            // player two makes his turn
            playTurnAndCheck(2);
            // TODO: should wait until first player waits on CV
        }
    }

    void playTurn(user player) {
        TicTacToeMessage turnStartedMessage;
        turnStartedMessage.status = status.turnStarted;
        // TODO: pass also current board state
        sendMessage(this->socket, player.connectionDesc, turnStartedMessage);
        // TODO: accept only playTurn command
        waitForValidCommand(this->socket, player.connectionDesc, command.playTurn);
        // TODO: make changes to board
        // TODO: send accepted message
    }
    void playGame(user player) {
        // waiting until the game is started
        std::mutex gameStartedMutex;
        std::unique_lock<std::mutex> lock(gameStartedMutex);
        gameStartCV.wait(lock);
        this->print("Client" + std::to_string(player.id) + " thread was notified." , true);

        // sending game started message to client
        TicTacToeMessage gameStartedMessage;
        gameStartedMessage.status = status.gameStarted;
        sendMessage(this->socket, player.connectionDesc, gameStartedMessage);

        while (true) {
            // waiting on condition_variable instance
            // until gameScheduler thread will notify it
            waitForTurnStart(player);
            this->print("Client" + std::to_string(player.id) + " thread was notified to start", true);

            playTurn(player);
            // notifying the turn is finished
            // accepted by gameScheduler
            notifyTurnEnd(player);
        }
    }

    void serveClient(int connectionDesc) {
        user currentUser{};
        do {
            currentUser = this->handleNewUser(connectionDesc);
        } while (currentUser.type == user_type.noType);
        this->print("New user was logged in: " + getUserRepr(currentUser), true);

        if (currentUser.type == user_type.exited) return;
        if (currentUser.type == user_type.player) {
            this->playGame(currentUser);
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
        std::thread t1(&GameServer::gameScheduler, this);

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