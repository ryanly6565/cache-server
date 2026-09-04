#include "cache_server/command.hpp"

class RequestParser {
public:
    // parses string to find commands
    ParseResult parse(const std::string& request) const;

    // converts parse errors to error messages
    std::string parse_error_message(ParseError error);
};