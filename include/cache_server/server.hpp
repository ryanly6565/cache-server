#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <string>

#include "cache_server/store.hpp"
#include "cache_server/thread_pool.hpp"

class Server {
public:
    explicit Server(std::uint16_t port, 
                    std::size_t capacity = std::numeric_limits<std::size_t>::max(),
                    std::size_t worker_count = 4,
                    std::size_t max_pending_clients = 64);
    ~Server();

    // don't allow copying of servers
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void run();
    void stop();

private:
    std::uint16_t port_;
    int listening_socket_ = -1;
    Store store_{};                            // the data storage
    std::atomic<bool> running_{false};         // indicates if server should continue running
    std::unordered_set<int> client_sockets_;   // a set of client sockets
    std::mutex clients_mutex_;                 // lock for the socket list
    ThreadPool thread_pool_;                   // the worker pool

    // helper for ensuring all data is sent
    void send_all(int client_socket, const std::string& message);

    // helper for doing communication with a client
    void handle_client(int client_socket);
};