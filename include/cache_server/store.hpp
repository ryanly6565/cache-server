#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <cstddef>
#include <list>
#include <limits>
#include <stdexcept>
#include <iostream>

// Class representing the internal storage of data.
class Store {
public:
    // Constructor
    Store(): max_capacity_(std::numeric_limits<std::size_t>::max()) {};
    Store(std::size_t max_capacity) {
        if (max_capacity <= 0) {
            throw std::invalid_argument("Store capacity must be greater than zero.");
        }
        max_capacity_ = max_capacity;
    };

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
    // Getter function for current capacity
    std::size_t size() const;


    void print_data(){
        std::cout << "print dataa:\n";
        for (auto pairing : data_) {
            std::cout << pairing.first << " " << pairing.second.value << "\n";
        }
    }

private:
    // An entry in the cache, stores a value and an expiration time.
    struct Entry {
        std::string value;
        std::optional<std::chrono::time_point<std::chrono::steady_clock>> expiry_date;
        std::list<std::string>::iterator lru_position;
    };

    // The actual map that stores pairings.
    std::unordered_map<std::string, Entry> data_;

    // Capacity varaible
    std::size_t max_capacity_;

    // LRU variables
    std::list<std::string> lru_order_;
    
    // Lock for concurrency.
    mutable std::mutex mutex_;

    // upon a sucessful access, update the LRU list
    inline void update_entry(Entry& hit_entry) {
        lru_order_.splice(lru_order_.begin(), lru_order_, hit_entry.lru_position);
    }

    // checks if entry is expired and deletes it if it is, false means it is not expired
    inline bool erase_if_expired(std::pair<const std::string, Entry>& pairing) {
        Entry entry = pairing.second;
        if (entry.expiry_date.has_value() && entry.expiry_date.value() < std::chrono::steady_clock::now()) {
            lru_order_.erase(entry.lru_position);
            data_.erase(pairing.first);
            return true;
        }

        return false;
    }

    // loops through pairings and removes them
    void clean_expired() {
        auto position = data_.begin();

        while (position != data_.end()) {
            auto current = position;
            position++;
            erase_if_expired(*current);
        }
    }
};








