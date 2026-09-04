#include <iostream>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <thread>

#include "cache_server/server.hpp"
#include "cache_server/store.hpp"
#include "cache_server/command.hpp"
#include "cache_server/command_processor.hpp"
#include "cache_server/request_parser.hpp"

Server::Server(std::uint16_t port): port_(port) {};
Server::~Server() {
    if (listening_socket_ != -1) {
        close(listening_socket_);
    }
}

void Server::send_all(int client_socket, const std::string& message) {
    std::size_t bytes_sent = 0;
    while (bytes_sent < message.size()) {
        ssize_t curr_bytes_sent = send(client_socket, message.data() + bytes_sent, message.size() - bytes_sent, MSG_NOSIGNAL);

        if (curr_bytes_sent == -1) {
            throw std::system_error(errno, std::generic_category(), "Sending to the socket has failed.");
        }

        if (curr_bytes_sent == 0) {
            throw std::runtime_error("Socket closed during sending.");
        }

        bytes_sent += static_cast<std::size_t>(curr_bytes_sent);
    }
}

void Server::run() {
    // create a socket
    listening_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    
    // ensure we received a socket
    if (listening_socket_ == -1) {
        throw std::system_error(errno, std::generic_category(), "Socket creation has failed.");
    }

    // reuse ports
    int reuse_address = 1;
    if (setsockopt(listening_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse_address,sizeof(reuse_address)) 
        == -1) {
        throw std::system_error(errno, std::generic_category(), "Setting SO_REUSEADDR failed.");
    }


    // create the network address
    sockaddr_in socket_address{};
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port_);
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the socket and address
    if (bind(listening_socket_, (struct sockaddr*)&socket_address, sizeof(socket_address)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Socket binding has failed.");
    }

    // listen on the socket for connections
    if (listen(listening_socket_, 8) == -1) {
        throw std::system_error(errno, std::generic_category(), "Listening on the socket has failed.");
    }
    running_ = true;

    // accept any clients
    while (running_) {
        int client_socket = accept(listening_socket_, nullptr, nullptr);
        if (client_socket == -1) {
            if (!running_) break;
            throw std::system_error(errno, std::generic_category(), "Listening on the socket has failed.");
        }

        bool reject_client = false;
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);

            if (!running_) {
                reject_client = true;
            } else {
                client_sockets_.insert(client_socket);
            }
        }

        if (reject_client) {
            close(client_socket);
            break;
        }

        client_threads_.emplace_back([this, client_socket]() {
            try {
                handle_client(client_socket);
            } catch (const std::exception& error) {
                std::cerr << "Client error: " << error.what() << '\n';
            }

            // close the socket at the end of the thread
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                client_sockets_.erase(client_socket);
            }
            close(client_socket);
        });
    }

    // if there are still clients, wait for them to finish
    for (std::thread& client_thread : client_threads_) {
        if (client_thread.joinable()) {
            client_thread.join();
        }
    }

    client_threads_.clear();
}

void Server::handle_client(int client_socket) {
    // setup communication variables
    constexpr std::size_t MAX_REQUEST_SIZE = 4096;
    char receive_buffer[MAX_REQUEST_SIZE];
    std::string input_buffer = "";
    bool close_connection = false;

    // set up parsing variables
    RequestParser request_parser;
    CommandProcessor command_processor{store_};

    // communication loop
    while (!close_connection) {
        // receive text
        ssize_t bytes_received = recv(client_socket, receive_buffer, sizeof(receive_buffer), 0);

        // client disconnects
        if (bytes_received == 0) {
            break;
        }
        // error catching
        else if (bytes_received == -1) {
            int receive_error = errno;
            throw std::system_error(receive_error, std::generic_category(), "Receiving from the socket has failed.");
        }

        input_buffer.append(receive_buffer, static_cast<std::size_t>(bytes_received));

        // search for a new line
        size_t newline_index = input_buffer.find('\n');

        // keep searching for new lines, there could be multiple in the buffer
        while (newline_index != std::string::npos) {
            if (newline_index + 1 > MAX_REQUEST_SIZE) {
                std::string error_message = "Error: Message too large.\n";
                send_all(client_socket, error_message);
                close_connection = true;
                break;
            }

            std::string request = input_buffer.substr(0, newline_index);
            input_buffer.erase(0, newline_index + 1);
            newline_index = input_buffer.find('\n');

            // execute the request
            ParseResult result = request_parser.parse(request);
            std::string response;

            if (std::holds_alternative<Command>(result)) {
                response = command_processor.execute(std::get<Command>(result));
            }
            else {
                response = request_parser.parse_error_message(std::get<ParseError>(result));
            }
            
            // send back response
            response.push_back('\n');
            send_all(client_socket, response);
        }

        // check that they didnt exceed bounds
        if (!close_connection && input_buffer.size() >= MAX_REQUEST_SIZE) {
            const std::string error = "Error: Message too large.\n";
            send_all(client_socket, error);

            close_connection = true;
        }
    }
}

void Server::stop() {
    // indicate we should stop running
    running_ = false;

    // close the listening socket gracefully
    if (listening_socket_ != -1) {
        shutdown(listening_socket_, SHUT_RDWR);
        close(listening_socket_);
        listening_socket_ = -1;
    }

    // shutdown all sockets
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (int client_socket : client_sockets_) {
        shutdown(client_socket, SHUT_RDWR);
    }
}