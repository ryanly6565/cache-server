#include <sstream>
#include <string>
#include <climits>

#include "cache_server/request_parser.hpp"

ParseResult RequestParser::parse(const std::string& request) const {
    std::istringstream token_iterator(request);
    std::string command;
    token_iterator >> command;

    std::transform(
        command.begin(),
        command.end(),
        command.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::toupper(character));
        }
    );

    // parse a set command
    if (command == "SET") {
        std::string key, value;
        if (!(token_iterator >> key)) return ParseError::EMPTY_KEY;

        // whatever is after the key is taken as the value
        std::getline(token_iterator >> std::ws, value);
        if (value == "") return ParseError::EMPTY_VALUE;

        return Command {CommandType::SET, key, value, std::nullopt};
    }

    // parse a get command
    else if (command == "GET") {
        std::string key, extra;
        if (!(token_iterator >> key)) return ParseError::EMPTY_KEY;
        if (token_iterator >> extra) return ParseError::WRONG_ARGUMENT_COUNT;

        return Command {CommandType::GET, key, std::nullopt, std::nullopt};
    }

    // parse a delete command
    else if (command == "DELETE") {
        std::string key, extra;
        if (!(token_iterator >> key)) return ParseError::EMPTY_KEY;
        if (token_iterator >> extra) return ParseError::WRONG_ARGUMENT_COUNT;

        return Command {CommandType::DELETE, key, std::nullopt, std::nullopt};
    }

    // parse an exists command
    else if (command == "EXISTS") {
        std::string key, extra;
        if (!(token_iterator >> key)) return ParseError::EMPTY_KEY;
        if (token_iterator >> extra) return ParseError::WRONG_ARGUMENT_COUNT;

        return Command {CommandType::EXISTS, key, std::nullopt, std::nullopt};
    }

    // parse an expire command
    else if (command == "EXPIRE") {
        std::string key, duration_str, extra;
        
        if (!(token_iterator >> key)) return ParseError::EMPTY_KEY;
        if (!(token_iterator >> duration_str)) {
            return ParseError::INVALID_DURATION;
        }

        std::size_t characters_read = 0;
        long long duration;

        try {
            duration = std::stoll(duration_str, &characters_read);
        } catch (...) {
            return ParseError::INVALID_DURATION;
        }

        if (characters_read != duration_str.size() || duration <= 0) {
            return ParseError::INVALID_DURATION;
        }

        if (duration <= 0 || duration > INT_MAX) {
            return ParseError::INVALID_DURATION;
        }

        if (token_iterator >> extra) return ParseError::WRONG_ARGUMENT_COUNT;

        return Command {CommandType::EXPIRE, key, std::nullopt, std::chrono::seconds{duration}};
    }

    // error on empty command
    else if (command == "") {
        return ParseError::EMPTY_REQUEST;
    }

    // error on unknown commands
    else {
        return ParseError::UNKNOWN_COMMAND;
    }
}

// converts a ParseError type to an error message
std::string RequestParser::parse_error_message(ParseError error) {
    switch(error) {
        case (ParseError::EMPTY_REQUEST):
            return "Error: Empty request.";
        case (ParseError::UNKNOWN_COMMAND):
            return "Error: Unknown command.";
        case (ParseError::WRONG_ARGUMENT_COUNT):
            return "Error: Incorrect argument count.";
        case (ParseError::EMPTY_KEY):
            return "Error: Empty key.";
        case (ParseError::EMPTY_VALUE):
            return "Error: Empty value.";
        case (ParseError::INVALID_DURATION):
            return "Error: Invalid duration.";
        default:
            return "Error: Something went wrong.";
    }
}