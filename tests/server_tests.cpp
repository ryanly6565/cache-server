#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <chrono>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <stdexcept>
#include <system_error>

#include "cache_server/server.hpp"

// helper for setting up the store server
int connect_to_server(std::uint16_t port) {
    // create server address
    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    // connect to server
    for (int attempt = 0; attempt < 50; attempt++) {
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (client_socket == -1) {
            throw std::system_error(errno, std::generic_category(), "Client socket creation failed.");
        }

        if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) == 0) {
            return client_socket;
        }

        close(client_socket);
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    throw std::runtime_error("Could not connect to test server.");
}

// helper for sending whole line (normal send can send only part)
void send_line(int socket, const std::string& message) {
    std::size_t sent = 0;

    while (sent < message.size()) {
        ssize_t amount = send(socket, message.data() + sent, message.size() - sent, 0);

        if (amount <= 0) {
            throw std::runtime_error("Failed to send test request.");
        }

        sent += static_cast<std::size_t>(amount);
    }
}

// helper for receiving one sent line at a time
std::string receive_line(int client_socket, std::string& pending) {
    while (true) {
        std::size_t newline = pending.find('\n');

        if (newline != std::string::npos) {
            std::string line = pending.substr(0, newline + 1);
            pending.erase(0, newline + 1);
            return line;
        }

        char buffer[1024];
        ssize_t received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (received <= 0) {
            throw std::runtime_error("Connection closed while receiving.");
        }

        pending.append(buffer, static_cast<std::size_t>(received));
    }
}

// setup the test startup and cleanup
class ServerTest : public ::testing::Test {
protected:
    static constexpr std::uint16_t port = 18080;

    Server server{port};
    std::thread server_thread;
    int client_socket{-1};

    // startup
    void SetUp() override {
        server_thread = std::thread([this]() {
            server.run();
        });

        client_socket = connect_to_server(port);
    }

    // clean up
    void TearDown() override {
        if (client_socket != -1) {
            close(client_socket);
            client_socket = -1;
        }

        server.stop();

        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
};

// Test that we can send singular commands to the server.
TEST_F(ServerTest, SetCommandReturnsOk) {
    std::string command = "SET username new_user\n";
    send_line(client_socket, command);

    // check responses
    std::string pending;
    EXPECT_EQ(receive_line(client_socket, pending), "OK\n");
}

// Test that we can send multiple commands to the server in one send.
TEST_F(ServerTest, MultipleCommandsOneSendReturnsOk) {
    std::string commands =
        "SET username new_user\n"
        "GET username\n"
        "DELETE username\n"
        "GET username\n";
    send_line(client_socket, commands);

    // check responses
    std::string pending;
    EXPECT_EQ(receive_line(client_socket, pending), "OK\n");
    EXPECT_EQ(receive_line(client_socket, pending), "VALUE new_user\n");
    EXPECT_EQ(receive_line(client_socket, pending), "INTEGER 1\n");
    EXPECT_EQ(receive_line(client_socket, pending), "NOT_FOUND\n");
}

// Test that we can send multiple commands to the server from one client.
TEST_F(ServerTest, MultipleSendsOneCommandReturnsOk) {
    send_line(client_socket, "SET user ");
    send_line(client_socket, "new_user\n");

    // check responses
    std::string pending;
    EXPECT_EQ(receive_line(client_socket, pending), "OK\n");
}

// Test that failed commands sent to the server from one client return the appropiate message.
TEST_F(ServerTest, IncorrectCommandReturnsError) {
    send_line(client_socket, "SET user \n");                   // missing value
    send_line(client_socket, "Get user 2 \n");                 // extra arg
    send_line(client_socket, "replace user new_user \n");      // unknown command

    // check responses
    std::string pending;
    EXPECT_EQ(receive_line(client_socket, pending), "Error: Empty value.\n");
    EXPECT_EQ(receive_line(client_socket, pending), "Error: Incorrect argument count.\n");
    EXPECT_EQ(receive_line(client_socket, pending), "Error: Unknown command.\n");
}

// Test that over-sized commands sent to the server from one client return the appropiate message (new line in message).
TEST_F(ServerTest, OversizedCommandNewLineReturnsError) {
    std::string long_message = std::string(4096, 'k') + "\n";
    send_line(client_socket, long_message);

    // check responses
    std::string pending;
    EXPECT_EQ(receive_line(client_socket, pending), "Error: Message too large.\n");
}

// Test that over-sized commands sent to the server from one client return the appropiate message (new line not in message).
TEST_F(ServerTest, OversizedCommandReturnsError) {
    std::string long_message = std::string(4097, 'k');
    send_line(client_socket, long_message);

    // check responses
    std::string pending;
    EXPECT_EQ(receive_line(client_socket, pending), "Error: Message too large.\n");
}

// Test that two clients access the same store.
TEST_F(ServerTest, MultipleClientsSameStore) {
    // first client sends message
    send_line(client_socket, "SET car red\n");

    // check responses
    std::string client_1_pending;
    EXPECT_EQ(receive_line(client_socket, client_1_pending), "OK\n");

    // second client sends message
    int client_socket_2 = connect_to_server(port);
    send_line(client_socket_2, "GET car\n");

    // check responses
    std::string client_2_pending;
    EXPECT_EQ(receive_line(client_socket_2, client_2_pending), "VALUE red\n");

    if (client_socket_2 != -1) {
        close(client_socket_2);
        client_socket_2 = -1;
    }
}

// Test that two clients access the same store, even when one disconnects before the other.
TEST_F(ServerTest, MultipleClientsSequentialDisconnect) {
    // disconnecting client sends partial  message
    int disconnecting_client_socket = client_socket;
    send_line(disconnecting_client_socket, "SET car red\n");
    send_line(disconnecting_client_socket, "SET car blue");

    // check responses
    std::string disconnecting_client_pending;
    EXPECT_EQ(receive_line(disconnecting_client_socket, disconnecting_client_pending), "OK\n");

    // first client disconnects
    if (disconnecting_client_socket != -1) {
        close(disconnecting_client_socket);
        client_socket = -1;
    }

    //new client connects
    int new_client_socket = connect_to_server(port);

    // new client sends message
    send_line(new_client_socket, "GET car\n");

    // check responses
    std::string new_client_pending;
    EXPECT_EQ(receive_line(new_client_socket, new_client_pending), "VALUE red\n");

    if (new_client_socket != -1) {
        close(new_client_socket);
        new_client_socket = -1;
    }
}


// Test that two clients access the same store, even when one disconnects during the other's connection.
TEST_F(ServerTest, MultipleClientsConcurrentDisconnect) {
    // disconnecting client sends partial  message
    int disconnecting_client_socket = client_socket;
    send_line(disconnecting_client_socket, "SET car red\n");
    send_line(disconnecting_client_socket, "SET car blue");

    // check responses
    std::string disconnecting_client_pending;
    EXPECT_EQ(receive_line(disconnecting_client_socket, disconnecting_client_pending), "OK\n");

    //new client connects
    int new_client_socket = connect_to_server(port);

    // first client disconnects
    if (disconnecting_client_socket != -1) {
        close(disconnecting_client_socket);
        client_socket = -1;
    }

    // new client sends message
    send_line(new_client_socket, "GET car\n");

    // check responses
    std::string new_client_pending;
    EXPECT_EQ(receive_line(new_client_socket, new_client_pending), "VALUE red\n");

    if (new_client_socket != -1) {
        close(new_client_socket);
        new_client_socket = -1;
    }
}

// Test that no hanging occrurs if the server shuts down during a simultaneous connection.
TEST_F(ServerTest, ServerShutdownDoesntHang) {
    // second client connects
    int new_client_socket = connect_to_server(port);

    // server shuts down
    server.stop();

    // wait fot server thread to finish
    ASSERT_TRUE(server_thread.joinable());
    server_thread.join();

    // check that both clients are closed gracefully
    char buffer[1];
    ssize_t received = recv(client_socket, buffer, 1, 0);
    int receive_error = errno;
    EXPECT_TRUE(received == 0 || (received == -1 && receive_error == ECONNRESET));
    received = recv(new_client_socket, buffer, 1, 0);
    receive_error = errno;
    EXPECT_TRUE(received == 0 || (received == -1 && receive_error == ECONNRESET));

    // close sockets (and stop teardown from closing intial client again)
    close(client_socket);
    client_socket = -1;
    close(new_client_socket);
}

// Class for testing capacity
class CapacityServerTest : public ::testing::Test {
protected:
    static constexpr std::uint16_t port = 18081;

    Server server{port, 2};
    std::thread server_thread;
    int client_socket{-1};

    void SetUp() override {
        server_thread = std::thread([this]() {
            server.run();
        });

        client_socket = connect_to_server(port);
    }

    void TearDown() override {
        if (client_socket != -1) {
            close(client_socket);
        }

        server.stop();

        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
};

TEST_F(CapacityServerTest, ServerEvictsLeastRecentlyUsedKey) {
    send_line(client_socket, 
             "SET first one\n"
             "SET second two\n"
             "GET first\n"
             "SET third three\n"
             "GET first\n"
             "GET second\n"
             "GET third\n");
    std::string pending;

    EXPECT_EQ(receive_line(client_socket, pending), "OK\n");
    EXPECT_EQ(receive_line(client_socket, pending), "OK\n");
    EXPECT_EQ(receive_line(client_socket, pending), "VALUE one\n");
    EXPECT_EQ(receive_line(client_socket, pending), "OK\n");
    EXPECT_EQ(receive_line(client_socket, pending), "VALUE one\n");
    EXPECT_EQ(receive_line(client_socket, pending), "NOT_FOUND\n");
    EXPECT_EQ(receive_line(client_socket, pending), "VALUE three\n");

}

// Class for testing the worker pool
class WorkerPoolServerTest : public ::testing::Test {
protected:
    static constexpr std::uint16_t port = 18081;

    Server server{port, 100, 1, 1};
    std::thread server_thread;
    int client_socket{-1};

    void SetUp() override {
        server_thread = std::thread([this]() {
            server.run();
        });

        client_socket = connect_to_server(port);
    }

    void TearDown() override {
        if (client_socket != -1) {
            ::close(client_socket);
            client_socket = -1;
        }

        server.stop();

        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
};

// Test that a connection is rejected if worker and task pool is occupied
TEST_F(WorkerPoolServerTest, RejectsClientWhenWorkerAndQueueAreFull) {
    // client 1 occupies worker
    send_line(client_socket, "SET username Ryan\n");

    std::string first_pending;
    ASSERT_EQ(receive_line(client_socket, first_pending), "OK\n");

    // client 2 goes into queue
    int queued_client = connect_to_server(port);
    send_line(queued_client, "GET username\n");

    // client 3 fails to enter
    int rejected_client = connect_to_server(port);

    // timeout if connection breaks
    timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // we will not wait long for rejected client
    ASSERT_NE(setsockopt(rejected_client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)), -1);

    char buffer[1];
    std::string rejected_pending;
    EXPECT_EQ(receive_line(rejected_client, rejected_pending), "Error: server busy\n");

    // close the tcp socket and make sure it closes naturally
    errno = 0;
    ssize_t received = recv(rejected_client, buffer, sizeof(buffer), 0);
    int receive_error = errno;
    EXPECT_TRUE(received == 0 || (received == -1 && receive_error == ECONNRESET));
    close(rejected_client);

    // client 1 is released
    close(client_socket);
    client_socket = -1;

    // client 2 should be handled
    std::string second_pending;
    EXPECT_EQ(receive_line(queued_client, second_pending), "VALUE Ryan\n");
    close(queued_client);
}