#include <cache_server/store.hpp>

// Sets up a key-value pairing.
void Store::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_.insert_or_assign(key, Store::Entry {value, std::nullopt});
}

// Retrieve a value based on a given key.
std::optional<std::string> Store::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto search_result = data_.find(key);
    if (search_result == data_.end()) return std::nullopt;

    auto expiry_date = search_result->second.expiry_date;
    if (expiry_date.has_value() && expiry_date.value() < std::chrono::steady_clock::now()) {
        return std::nullopt;
    }

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
        data_.insert_or_assign(key, Store::Entry {search_result->second.value, std::chrono::steady_clock::now() + lifetime});
        return Store::ExpireResult::SUCCESS;
    }
    return Store::ExpireResult::KEY_NOT_FOUND;
}

