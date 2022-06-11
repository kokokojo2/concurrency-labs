#include <iostream>
#include <string>
#include <regex>
#include <vector>

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
} status;

bool isValidMessageStatus(int statusId) {
    return statusId == status.accepted || statusId == status.error || statusId == status.previousCommandInvalid;
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

    // parsing message status
    int parsedStatus = std::stoi(rawMessage);
    if(!isValidMessageStatus(parsedStatus)) {
        parsedMessage.valid = false;
        parsedMessage.errorMessage = "Invalid status.";
        return parsedMessage;
    }

    parsedMessage.valid = true;
    parsedMessage.status = parsedStatus;
    return parsedMessage;
}

std::string messageToString(const TicTacToeMessage& ticTacToeMessage) {
    return std::to_string(ticTacToeMessage.status);
}

void printMessage(const TicTacToeMessage& message) {
    std::cout << "TicTacToeMessage: valid=" << message.valid << std::endl;
    std::cout << "  raw: " << message.rawBody << std::endl;
    if (!message.valid) {
        std::cout << "  Error message: " << message.errorMessage << std::endl;
    } else {
        std::cout << "  statusId=" << message.status << std::endl;
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
};

std::string getUserRepr(user currentUser) {
    if (currentUser.type == user_type.player) return "type=player, id=" + std::to_string(currentUser.id);
    if (currentUser.type == user_type.spectator) return "type=spectator";
    if (currentUser.type == user_type.exited) return "type=exited";
    if (currentUser.type == user_type.noType) return "type=no type";
    return "unknown";
}

