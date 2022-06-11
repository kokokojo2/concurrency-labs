#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include "socket.h"
#include "constants.h"

std::vector<std::string> split_to_tokens(const std::string& string, char delimiter) {
    std::vector<std::string> strings;
    std::istringstream f(string);
    std::string s;
    while (getline(f, s, delimiter)) {
        strings.push_back(s);
    }
    return strings;
}

struct commandEnum {
    int connectToGame = 1;
    int exitGame = 2;
    int playTurn = 3;
    int watchGame = 4;
    int disconnect = 5;
} command;

std::string getReprCommand(int commandId) {
    if (commandId == command.connectToGame) return "play game";
    if (commandId == command.exitGame) return "exit game";
    if (commandId == command.playTurn) return "play turn";
    if (commandId == command.watchGame) return "watch current game";
    if (commandId == command.disconnect) return "disconnect";
    return "unknown";
}

bool isValidCommand(int commandId) {
    return commandId == command.connectToGame ||
           commandId == command.exitGame ||
           commandId == command.playTurn ||
           commandId == command.watchGame ||
           commandId == command.disconnect;
}

struct statusEnum {
    int accepted = 1;
    int error = 2;
    int previousCommandInvalid = 3;
    int gameStarted = 4;
    int turnStarted = 5;
    int currentBoard = 6;
} status;

bool isValidMessageStatus(int statusId) {
    return statusId == status.accepted ||
    statusId == status.error ||
    statusId == status.previousCommandInvalid ||
    statusId == status.gameStarted ||
    statusId == status.turnStarted ||
    statusId == status.currentBoard;
}

struct TicTacToeCommand {
    std::string rawBody;
    bool valid = true;
    int command{};
    int xCoord{};
    int yCoord{};
    std::string errorMessage{};
};


struct TicTacToeMessage {
    std::string rawBody;
    bool valid{};
    std::string errorMessage{};
    int status{};
    std::vector<std::vector<int>> boardState;
};


TicTacToeCommand parseRequest(const std::string& rawCommand) {
    TicTacToeCommand parsedCommand;
    parsedCommand.rawBody = rawCommand;

    std::regex pattern("^([0-9]):(.*)$");
    std::smatch matches;
    if(!std::regex_search(rawCommand, matches, pattern)) {
        parsedCommand.valid = false;
        parsedCommand.errorMessage = "Syntax error in request body.";
        return parsedCommand;
    }

    // parsing command name
    int parsedCommandId = std::stoi(matches[1].str());
    if(!isValidCommand(parsedCommandId)) {
        parsedCommand.valid = false;
        parsedCommand.errorMessage = "Invalid command.";
        return parsedCommand;
    }
    parsedCommand.command = parsedCommandId;

    // parsing params for specific commands
    std::vector<std::string> params = split_to_tokens(matches[2].str(), ',');
    if(parsedCommand.command == command.playTurn) {
        if (params.size() != 2) {
            parsedCommand.valid = false;
            parsedCommand.errorMessage = "Wrong params for this command type.";
            return parsedCommand;
        }
        parsedCommand.xCoord = std::stoi(params[0]);
        parsedCommand.yCoord = std::stoi(params[1]);

        if (parsedCommand.xCoord >= BOARD_SIZE || parsedCommand.yCoord >= BOARD_SIZE) {
            parsedCommand.valid = false;
            parsedCommand.errorMessage = "Wrong params for this command type.";
            return parsedCommand;
        }
    }

    return parsedCommand;
}

std::string commandToString(const TicTacToeCommand& ticTacToeCommand) {
    std::string result = std::to_string(ticTacToeCommand.command) + ":";
    if(ticTacToeCommand.command == command.playTurn) {
        result += std::to_string(ticTacToeCommand.xCoord) +
                  "," + std::to_string(ticTacToeCommand.yCoord);
    }

    return result;
}

void printCommand(const TicTacToeCommand& request) {
    std::cout << "TicTacToeCommand: valid=" << request.valid << std::endl;
    std::cout << "  raw: " << request.rawBody << std::endl;
    if (!request.valid) {
        std::cout << "  Error message: " << request.errorMessage << std::endl;
    } else {
        std::cout << "  commandId=" << request.command << std::endl;
        if (request.command == command.playTurn) {
            std::cout << "  xPosition=" << request.xCoord << " " << "yPosition=" << request.yCoord << std::endl;
        }
    }
}

TicTacToeMessage parseResponse(const std::string& rawMessage) {
    TicTacToeMessage parsedMessage;
    parsedMessage.rawBody = rawMessage;

    std::regex pattern("^([0-9]):(.*)$");
    std::smatch matches;
    if(!std::regex_search(rawMessage, matches, pattern)) {
        parsedMessage.valid = false;
        parsedMessage.errorMessage = "Syntax error in message body.";
        return parsedMessage;
    }

    int parsedMessageStatus = std::stoi(matches[1].str());
    if(!isValidMessageStatus(parsedMessageStatus)) {
        parsedMessage.valid = false;
        parsedMessage.errorMessage = "Invalid status.";
        return parsedMessage;
    }
    parsedMessage.status = parsedMessageStatus;

    if(parsedMessage.status == status.currentBoard) {
        std::vector<std::string> params = split_to_tokens(matches[2].str(), ',');
        if (params.size() != BOARD_SIZE * BOARD_SIZE) {
            parsedMessage.valid = false;
            parsedMessage.errorMessage = "Wrong params for this command type.";
            return parsedMessage;
        }
        std::vector<std::vector<int>> board;
        for(int i = 0; i < params.size(); i+= BOARD_SIZE) {
            std::vector<int> boardRow = {
                    std::stoi(params[i]),
                    std::stoi(params[i + 1]),
                    std::stoi(params[i + 2])
            };
            board.push_back(boardRow);
        }
        parsedMessage.boardState = board;
    }


    parsedMessage.valid = true;
    return parsedMessage;
}

std::string boardToString(std::vector<std::vector<int>> board) {
    std::string serializedBoard;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            serializedBoard += std::to_string(board[i][j]) + ",";
        }
    }
    serializedBoard.pop_back();
    return serializedBoard;
}

std::string  getCellRepr(int cellValue) {
    if (cellValue == 0) return "-";
    if (cellValue == 1) return "x";
    if (cellValue == 2) return "o";
    return "unknown";
}

void printBoard(std::vector<std::vector<int>> board) {
    std::cout << "Current game state:" << std::endl;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            std::cout << getCellRepr(board[i][j]) << " ";
        }
        std::cout << std::endl;
    }
}


std::string messageToString(const TicTacToeMessage& ticTacToeMessage) {
    std::string serializedMessage =  std::to_string(ticTacToeMessage.status) + ":";
    if (ticTacToeMessage.status == status.currentBoard) {
        serializedMessage += boardToString(ticTacToeMessage.boardState);
    }
    return serializedMessage;
}

void printMessage(const TicTacToeMessage& message) {
    auto message1 = parseResponse(messageToString(message));
    std::cout << "TicTacToeMessage: valid=" << message1.valid << std::endl;
    std::cout << "  raw: " << message1.rawBody << std::endl;
    if (!message1.valid) {
        std::cout << "  Error message: " << message1.errorMessage << std::endl;
    } else {
        std::cout << "  statusId=" << message1.status << std::endl;
    }
}


struct userType {
    int player = 1;
    int spectator = 2;
    int exited = -1;
    int noType = 0;
} user_type;

struct user {
    int type;
    int id;
    int connectionDesc;
};

std::string getUserRepr(user currentUser) {
    if (currentUser.type == user_type.player) return "type=player, id=" + std::to_string(currentUser.id);
    if (currentUser.type == user_type.spectator) return "type=spectator";
    if (currentUser.type == user_type.exited) return "type=exited";
    if (currentUser.type == user_type.noType) return "type=no type";
    return "unknown";
}

// wrappers to send/get protocol objects easily
// client functions
void writeCommand(ClientSocket socket, const TicTacToeCommand& ticTacToeCommand) {
    socket.writeMessage(commandToString(ticTacToeCommand));
}

TicTacToeMessage waitForMessage(ClientSocket socket) {
    while (true) {
        std::string message = socket.readMessage();
        if (!message.empty()) return parseResponse(message);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TicTacToeMessage waitForValidMessage(ClientSocket socket) {
    while (true) {
        auto message = waitForMessage(socket);
        if (!message.valid) {
            std::cout << "Got invalid message. This is likely to be the problem with server.";
            printMessage(message);
            continue;
        }
        return message;
    }
}

TicTacToeMessage waitForValidMessage(ClientSocket socket, int expectedStatus) {
    while (true) {
        auto message = waitForMessage(socket);
        if (!message.valid || message.status != expectedStatus) {
            std::cout << "Got invalid message. This is likely to be the problem with server." << std::endl;
            printMessage(message);
            continue;
        }
        return message;
    }
}

// server functions
void sendMessage(ServerSocket socket, int connectionDesc, const TicTacToeMessage&  message) {
    socket.sendMessage(connectionDesc, messageToString(message));
}

TicTacToeCommand waitForCommand(ServerSocket socket, int connectionDesc) {
    while (true) {
        std::string message = socket.getMessage(connectionDesc);
        if (!message.empty()) return parseRequest(message);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TicTacToeCommand waitForValidCommand(ServerSocket socket, int connectionDesc) {
    while (true) {
        auto parsedCommand = waitForCommand(socket, connectionDesc);
        if (parsedCommand.valid) return parsedCommand;

        std::cout << "Got invalid command. This is likely to be the problem with client." << std::endl;
        printCommand(parsedCommand);

        TicTacToeMessage messageInvalidCommand;
        messageInvalidCommand.status = status.previousCommandInvalid;
        sendMessage(socket, connectionDesc, messageInvalidCommand);
    }
}

TicTacToeCommand waitForValidCommand(ServerSocket socket, int connectionDesc, int expectedCommand) {
    while (true) {
        auto parsedCommand = waitForCommand(socket, connectionDesc);
        if (parsedCommand.valid && parsedCommand.command == expectedCommand) return parsedCommand;

        std::cout << "Got invalid command. This is likely to be the problem with client." << std::endl;
        printCommand(parsedCommand);

        TicTacToeMessage messageInvalidCommand;
        messageInvalidCommand.status = status.previousCommandInvalid;
        sendMessage(socket, connectionDesc, messageInvalidCommand);
    }
}