#include <iostream>
#include <cstring>
#include <cstdint>
#include <string>

#include "cache_server/server.hpp"

int main(int argc, char** argv) {
    std::uint16_t port = 8080;

    // take the user's input port number if provided
    if (argc > 1) {
        // convert the given number to an int, and catch any non-numbers
        std::size_t characters_read = 0;
        int potential_port = -1;
        try {
            potential_port = std::stoi(argv[1], &characters_read);
        } catch (...) {
            std::cerr << "Usage: Provided port number is not a valid port number" << std::endl;
            return 1;
        }
        if (characters_read != std::strlen(argv[1])) {
            std::cerr << "Usage: Provided port number is not a valid port number" << std::endl;
            return 1;
        }

        // check the port is valid
        if (potential_port < 1 || potential_port > 65535) {
            std::cerr << "Usage: Provided port number (" << potential_port << ") is not a valid port number" << std::endl;
            return 1;
        }

        port = static_cast<std::uint16_t>(potential_port);
    }

    // run the server
    Server server{port};
    std::cout << "Cache server starting on port " << port << std::endl;
    try {
        server.run();
    } catch (const std::exception& error) {
        std::cerr << "Server error: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}