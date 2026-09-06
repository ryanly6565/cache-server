#include <iostream>
#include <cache_server/store.hpp>

// Constructors
Store::Store(): Store(
          std::numeric_limits<std::size_t>::max(),
          std::chrono::milliseconds{1000}
      ) {}

Store::Store(std::size_t max_capacity, std::chrono::milliseconds cleanup_interval): 
             max_capacity_(max_capacity),
             cleanup_interval_(cleanup_interval) {
    if (max_capacity <= 0) {
        throw std::invalid_argument("Store capacity must be greater than zero.");
    }

    // cleanup thread
    if (cleanup_interval <= std::chrono::milliseconds{0}) {
        throw std::invalid_argument(
            "Cleanup interval must be greater than zero."
        );
    }

    cleanup_thread_ = std::thread([this]() {
        cleanup_loop();
    });
};

// Destructor
Store::~Store() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }

    cleanup_condition_.notify_all();

    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
}

// Sets up a key-value pairing.
void Store::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto search_result = data_.find(key);

    // if we are inserting rather than replacing
    if (search_result == data_.end()) {
        // try to do cleanup if needed
        if (data_.size() == max_capacity_) {
            // try to cleanup expired nodes
            clean_expired_locked();
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

    if (erase_if_expired(*search_result)) {
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

    if (erase_if_expired(*search_result)) {
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

    auto search_result = data_.find(key);

    if (search_result == data_.end()) {
        return Store::ExpireResult::KEY_NOT_FOUND;
    }

    // don't give expiry an already expired key
    if (erase_if_expired(*search_result)) {
        return Store::ExpireResult::KEY_NOT_FOUND;
    }

    search_result->second.expiry_date = std::chrono::steady_clock::now() + lifetime;
    return Store::ExpireResult::SUCCESS;
}

std::int64_t Store::ttl(const std::string& key) {
    static constexpr std::int64_t TTL_KEY_NOT_FOUND = -2;
    static constexpr std::int64_t TTL_NO_EXPIRATION = -1;

    std::lock_guard<std::mutex> lock(mutex_);
    auto search_result = data_.find(key);

    if (search_result == data_.end()) return TTL_KEY_NOT_FOUND;

    if (erase_if_expired(*search_result)) {
        return TTL_KEY_NOT_FOUND;
    }

    if (search_result->second.expiry_date.has_value()) {
        auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
            search_result->second.expiry_date.value() - std::chrono::steady_clock::now()
        );

        return static_cast<std::int64_t>(remaining.count());
    }

    return TTL_NO_EXPIRATION;
}

// Getter for current cacpcity.
std::size_t Store::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}

// loop for clean up thread
void Store::cleanup_loop() {
    std::unique_lock<std::mutex> lock(mutex_);

    while (!stopping_) {
        cleanup_condition_.wait_for(lock, cleanup_interval_, [this]() {
                return stopping_;
            }
        );

        if (stopping_) {
            break;
        }

        clean_expired_locked();
    }
}

// actual method that cleans up expired variables
void Store::clean_expired_locked() {
    auto iterator = data_.begin();
    const auto now = std::chrono::steady_clock::now();

    while (iterator != data_.end()) {
        Entry& entry = iterator->second;

        if (entry.expiry_date.has_value() && entry.expiry_date.value() <= now) {
            lru_order_.erase(entry.lru_position);
            iterator = data_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}