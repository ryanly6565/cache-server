#include <iostream>
#include <cache_server/store.hpp>

// Sets up a key-value pairing.
void Store::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto search_result = data_.find(key);

    // if we are inserting rather than replacing
    if (search_result == data_.end()) {
            std::cout << "1\n";
        // try to do cleanup if needed
        if (data_.size() == max_capacity_) {
            // try to cleanup expired nodes
            clean_expired();
        }

        // if the map is still full, we need to do cleanup
        if (data_.size() == max_capacity_) {
            std::string evicted_key = *(--lru_order_.end());
            lru_order_.erase(--lru_order_.end());
            data_.erase(evicted_key);
        }
        lru_order_.push_front(key);
        data_.insert_or_assign(key, Entry {value, std::nullopt, lru_order_.begin()});
    }

    // if we are replacing
    else {
        Entry& entry = search_result->second;
        entry.value = value;
        entry.expiry_date = std::nullopt;
        Store::update_entry(entry);
    }
}

// Retrieve a value based on a given key.
std::optional<std::string> Store::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto search_result = data_.find(key);
    if (search_result == data_.end()) return std::nullopt;

    if (erase_if_expired(*search_result)) {
        return std::nullopt;
    }

    Store::update_entry(search_result->second);
    return search_result->second.value;
}

// Remove a key-value pairing based on the given key.
bool Store::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto search_result = data_.find(key);

    if (search_result == data_.end()) return false;

    auto expiry_date = search_result->second.expiry_date;
    if (expiry_date.has_value() && expiry_date.value() < std::chrono::steady_clock::now()) {
        return false;
    }

    lru_order_.erase(search_result->second.lru_position);
    data_.erase(key);
    return true;
}

// Check for the exsitence of a key-value pairing.
bool Store::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto search_result = data_.find(key);

    if (search_result == data_.end()) return false;

    auto expiry_date = search_result->second.expiry_date;
    if (expiry_date.has_value() && expiry_date.value() < std::chrono::steady_clock::now()) {
        return false;
    }

    return true;
}

// Add an expiration date to a key-value pairing.
Store::ExpireResult Store::expire(const std::string& key, std::chrono::steady_clock::duration lifetime) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lifetime <= static_cast<std::chrono::seconds>(0)) {
        return Store::ExpireResult::INVALID_DURATION;
    }

    if (auto search_result = data_.find(key); search_result != data_.end()) {
        data_.insert_or_assign(key, Store::Entry {search_result->second.value, std::chrono::steady_clock::now() + lifetime, search_result->second.lru_position});
        return Store::ExpireResult::SUCCESS;
    }
    return Store::ExpireResult::KEY_NOT_FOUND;
}


// Getter for current cacpcity.
std::size_t Store::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}