#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <chrono>
#include <mutex>

// Class representing the internal storage of data.
class Store {
public:
    // Records the results of using an expire.
    enum class ExpireResult {
        SUCCESS,
        KEY_NOT_FOUND,
        INVALID_DURATION
    };

    // Operation for setting a key-value pairing.
    void set(const std::string& key, const std::string& value);
    // Operation for retriving the value from a given key. Cannot modify the data.
    std::optional<std::string> get(const std::string& key);
    // Operation for removing a key-value pairing.
    bool remove(const std::string& key);
    // Operation for checking if a key-value pairing exists. Cannot modify the data.
    bool exists(const std::string& key);
    // Operation for assigning an expiration to a key, given a positive integer representing time. Returns true if key is there, false otherwise.
    Store::ExpireResult expire(const std::string& key, std::chrono::steady_clock::duration lifetime);
private:
    // An entry in the cache, stores a value and an expiration time.
    struct Entry {
        std::string value;
        std::optional<std::chrono::time_point<std::chrono::steady_clock>> expiry_date;
    };

    // The actual map that stores pairings.
    std::unordered_map<std::string, Store::Entry> data_;

    // Lock for concurrency.
    std::mutex mutex_;
};








