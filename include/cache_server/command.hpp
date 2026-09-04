#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <variant>

// enum represetting the possible commands
enum class CommandType {
    SET,
    GET,
    DELETE,
    EXISTS,
    EXPIRE
};

// enum representing the possibel errors that can occur during parsing
enum class ParseError {
    EMPTY_REQUEST,
    UNKNOWN_COMMAND,
    WRONG_ARGUMENT_COUNT,
    EMPTY_KEY,
    EMPTY_VALUE,
    INVALID_DURATION
};

// Class representing a received command.
struct Command {
    CommandType type;
    std::string key;
    std::optional<std::string> value;
    std::optional<std::chrono::seconds> lifetime;

    // equality for commands
    friend bool operator==(Command command1, Command command2) {
        if (command1.type == command2.type &&
            command1.key == command2.key &&
            command1.value == command2.value &&
            command1.lifetime == command2.lifetime) {
                return true;
            }
        return false;
    }
};

using ParseResult = std::variant<Command, ParseError>;
