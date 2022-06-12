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

class GameServer : public Logger {
private:
    // communication data
    ServerSocket socket;
    int playerOneConnectionDesc = -1;
    int playerTwoConnectionDesc = -1;

    // 0 - cell is empty
    // 1 - user with id=1 owns the cell
    // 2 - user with id=1 owns the cell
    std::vector<std::vector<int>> board;

    // 0 - game is in progress
    // 1 - player 1 won the game
    // 2 - player 2 won the game
    // -1 - all cells are full, but there is no winner
    int winnerId = 0;


    // board methods
    void clearBoard() {
        this->board = constructBoard();
    }

    static std::vector<std::vector<int>> constructBoard() {
        std::vector<std::vector<int>> newBoard;
        for (int i = 0; i < BOARD_SIZE; i++) {
            std::vector<int> boardRow = {0, 0, 0};
            newBoard.push_back(boardRow);
        }
        return newBoard;
    }

    // player methods
    void setPlayerOneConnection(int connectionDesc) {
        this->playerOneConnectionDesc = connectionDesc;
    }

    void setPlayerTwoConnection(int connectionDesc) {
        this->playerTwoConnectionDesc = connectionDesc;
    }

    void clearPlayers() {
        std::lock_guard<std::mutex> lock(resourcesMutex);
        this->playerOneConnectionDesc = -1;
        this->playerTwoConnectionDesc = -1;
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
                message.status = status.connectedAsPlayer;
                message.userId = currentUser.id;
            }
        }

        sendMessage(this->socket, connectionDesc, message);
        currentUser.connectionDesc = connectionDesc;
        return currentUser;
    }

    // scheduler methods
    bool canGameStart() const {
        return this->playerTwoConnectionDesc != -1 && this->playerOneConnectionDesc != -1;
    }

    void waitForGameStart() {
        while (true) {
            if (this->canGameStart()) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    bool isGameFinished() {
        return this->winnerId != 0;
    }

    bool checkRow(std::vector<int> row, int playerId) {
        int win = 0;
        for(int i = 0; i < row.size(); i++) {
            if (row[i] == playerId) win += 1;
        }
        return win == row.size();
    }

    std::vector<std::vector<int>> getPossibleRows() {
        std::vector<std::vector<int>> possibleRows;

        possibleRows.push_back(this->board[0]);
        possibleRows.push_back(this->board[1]);
        possibleRows.push_back(this->board[2]);

        possibleRows.push_back(std::vector<int> {this->board[0][0], this->board[1][0], this->board[2][0]});
        possibleRows.push_back(std::vector<int> {this->board[0][1], this->board[1][1], this->board[2][1]});
        possibleRows.push_back(std::vector<int> {this->board[0][2], this->board[1][2], this->board[2][2]});

        possibleRows.push_back(std::vector<int> {this->board[0][0], this->board[1][1], this->board[2][2]});
        possibleRows.push_back(std::vector<int> {this->board[0][2], this->board[1][1], this->board[2][0]});

        return possibleRows;
    }
    bool checkWinCondition(int playerId) {
        auto possibleRows = this->getPossibleRows();
        for (int i = 0; i < possibleRows.size(); i++) {
            bool win = checkRow(possibleRows[i], playerId);
            if (win) return win;
        }
        return false;
    }

    bool isBoardFull() {
        for(int i = 0; i < BOARD_SIZE; i++) {
            for(int j = 0; j < BOARD_SIZE; j++) {
                if (!board[i][j]) return false;
            }
        }
        return true;
    }

    void playTurnAndCheckWinCondition(int playerId) {
        this->print("Player" + std::to_string(playerId) + " plays turn!", true);
        if (playerId == 1) {
            playerOneCanPlayTurnCV.notify_all();
            std::unique_lock<std::mutex> lock(playerOneFinishedPlayingTurnMutex);
            playerOneFinishedPlayingTurnCV.wait(lock);
        }
        if (playerId == 2) {
            playerTwoCanPlayTurnCV.notify_all();
            std::unique_lock<std::mutex> lock(playerTwoFinishedPlayingTurnMutex);
            playerTwoFinishedPlayingTurnCV.wait(lock);
        }
        this->print("Finished", true);
        printBoard(this->board);

        bool win = this->checkWinCondition(playerId);
        if (win) {
            this->winnerId = playerId;
            this->print("The game ended. User with id=" + std::to_string(playerId) + " has won!", true);
        }
        if (!win && this->isBoardFull()) {
            this->winnerId = -1;
            this->print("The game ended. No one wins this time. It's tie.", true);
        }
    }

    void playUntilWin() {
        while (true) {
            // waiting to ensure threads that handle turns started waiting for event
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            // player one makes his turn
            this->playTurnAndCheckWinCondition(1);
            if(this->isGameFinished()) break;
            // player two makes his turn
            this->playTurnAndCheckWinCondition(2);
            if(this->isGameFinished()) break;
        }
    }

    void scheduleGame() {
        this->waitForGameStart();
        // sleeping here to avoid notifying the condition_variable
        // before serveClient actually starts waiting for it
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        this->clearBoard();
        this->winnerId = 0;

        this->print("All players are ready. Starting the game!", true);

        // notifying serveClient that game is started
        gameStartCV.notify_all();
        this->playUntilWin();

        // serveClient threads wait for new turn, so
        // they have to be notified in order to initialize
        // closing procedure
        playerOneCanPlayTurnCV.notify_all();
        playerTwoCanPlayTurnCV.notify_all();

        this->clearPlayers();
    }

    void gameScheduler() {
        while (true) {
            scheduleGame();
        }
    }

    // serveClient methods
    void playTurn(user player) {
        TicTacToeMessage currentBoardMessage;
        currentBoardMessage.status = status.currentBoard;
        currentBoardMessage.boardState = this->board;

        sendMessage(this->socket, player.connectionDesc, currentBoardMessage);

        auto playTurnCommand = waitForValidCommand(this->socket, player.connectionDesc, command.playTurn);
        this->updateBoard(playTurnCommand.xCoord, playTurnCommand.yCoord, player.id);

        TicTacToeMessage acceptedMessage;
        acceptedMessage.status = status.accepted;
        sendMessage(this->socket, player.connectionDesc, acceptedMessage);
    }

    void waitForTurnStart(int playerId) const {
        if (playerId == 1) {
            std::unique_lock<std::mutex> lock(playerOneTurnStartMutex);
            playerOneCanPlayTurnCV.wait(lock);
        }
        if (playerId == 2) {
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

    void updateBoard(int xCoord, int yCoord, int value) {
        this->board[xCoord][yCoord] = value;
    }

    void playGame(user player) {
        // waiting until the game is started
        std::mutex gameStartedMutex;
        std::unique_lock<std::mutex> lock(gameStartedMutex);
        gameStartCV.wait(lock);

        // sending game started message to client
        TicTacToeMessage gameStartedMessage;
        gameStartedMessage.status = status.gameStarted;
        sendMessage(this->socket, player.connectionDesc, gameStartedMessage);

        while (true) {
            // waiting on condition_variable instance
            // until gameScheduler thread will notify it
            this->waitForTurnStart(player.id);
            if (isGameFinished()) break;
            this->playTurn(player);
            // notifying the turn is finished
            // waited on by gameScheduler
            this->notifyTurnEnd(player);
        }

        TicTacToeMessage endOfGameMessage;
        endOfGameMessage.status = status.gameFinished;
        endOfGameMessage.winnerId = this->winnerId;
        endOfGameMessage.boardState = this->board;
        sendMessage(this->socket, player.connectionDesc, endOfGameMessage);
    }
    void sendGameProgress(user spectator, int playerId) {
        TicTacToeMessage gameProgressMessage;
        gameProgressMessage.status = status.gameProgress;
        gameProgressMessage.userId = playerId;
        gameProgressMessage.boardState = board;

        sendMessage(this->socket, spectator.connectionDesc, gameProgressMessage);
    }

    void spectateGame(user spectator) {
        while (true) {
            this->waitForTurnStart(1);
            if(isGameFinished()) break;
            sendGameProgress(spectator, 1);
            this->waitForTurnStart(2);
            if(isGameFinished()) break;
            sendGameProgress(spectator, 2);
        }
        TicTacToeMessage gameFinishedMessage;
        gameFinishedMessage.status = status.gameFinished;
        gameFinishedMessage.boardState = board;
        gameFinishedMessage.winnerId = this->winnerId;
        sendMessage(this->socket, spectator.connectionDesc, gameFinishedMessage);
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
            this->spectateGame(currentUser);
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
        }
    };
};

int main() {
    auto server = GameServer();
    server.run();
    return 0;
}