#include <iostream>
#include <cstring>
#include <cstdint>
#include <string>
#include <limits>

#include "cache_server/server.hpp"

bool get_int(const char* input_string, int* potential_number, const std::string& context = "int") {
        // convert the given number to an int, and catch any non-numbers
        std::size_t characters_read = 0;
        int potential_int = -1;
        try {
            potential_int = std::stoi(input_string, &characters_read);
        } catch (...) {
            std::cerr << "Usage: Provided " << context << " is not a valid " << context << std::endl;
            return false;
        }
        if (characters_read != std::strlen(input_string)) {
            std::cerr << "Usage: Provided " << context << " is non-numeric" << std::endl;
            return false;
        }

        *potential_number = potential_int;
        return true;
}

int main(int argc, char** argv) {
    if (argc > 5) {
        std::cerr << "Usage: cache_server [port] [cache_capacity] [worker_count] [queue_capacity]\n";
        return 1;
    }

    std::uint16_t port = 8080;
    std::size_t capacity = std::numeric_limits<std::size_t>::max();
    std::size_t worker_count = 4;
    std::size_t max_pending_clients = 64;

    // take the user's input port number if provided
    if (argc > 1) {
        // convert the given number to an int, and catch any non-numbers
        int potential_port;
        if (!get_int(argv[1], &potential_port, "port number")) {
            return 1;
        }

        // check the port is valid
        if (potential_port < 1 || potential_port > 65535) {
            std::cerr << "Usage: Provided port number (" << potential_port << ") is not a valid port number" << std::endl;
            return 1;
        }

        port = static_cast<std::uint16_t>(potential_port);
    }

    if (argc > 2) {
        // convert the given number to an int, and catch any non-numbers
        int potential_capacity;
        if (!get_int(argv[2], &potential_capacity, "capacity")) {
            return 1;
        }

        // check the port is valid
        if (potential_capacity < 1) {
            std::cerr << "Usage: Provided capacity (" << potential_capacity << ") is not a valid capacity" << std::endl;
            return 1;
        }

        capacity = static_cast<std::size_t>(potential_capacity);
    }

    if (argc > 3) {
        int potential_count;
        if (!get_int(argv[3], &potential_count, "worker count")) {
            return 1;
        }

        // check the port is valid
        if (potential_count < 1) {
            std::cerr << "Usage: Provided worker count (" << potential_count << ") is not a valid worker count" << std::endl;
            return 1;
        }

        worker_count = static_cast<std::size_t>(potential_count);
    }

    if (argc > 4) {
        int potential_queue_capacity;
        if (!get_int(argv[4], &potential_queue_capacity, "queue capacity")) {
            return 1;
        }

        // check the port is valid
        if (potential_queue_capacity < 1) {
            std::cerr << "Usage: Provided queue capacity (" << potential_queue_capacity << ") is not a valid queue capacity" << std::endl;
            return 1;
        }

        max_pending_clients = static_cast<std::size_t>(potential_queue_capacity);
    }

    // run the server
    try {
        Server server{port, capacity, worker_count, max_pending_clients};

        std::cout << "Cache server starting on port " << port << '\n';

        server.run();
    } catch (const std::exception& error) {
        std::cerr << "Server error: " << error.what() << '\n';
        return 1;
    }
    
    return 0; 
}
