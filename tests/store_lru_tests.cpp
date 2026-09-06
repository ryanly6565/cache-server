#include <gtest/gtest.h>
#include <atomic>
#include <barrier>
#include <thread>
#include <vector>

#include "cache_server/store.hpp"

// Test that new Store cannot have 0 capacity
TEST(StoreLRUTest, ZeroCapacityThrows) {
    EXPECT_THROW(Store store{0}, std::invalid_argument);
}

// Test that a Store with 1 cacpacity accepts exactly 1 pairing.
TEST(StoreLRUTest, CapacityOneAcceptsOneKey) {
    Store store{1};
    store.set("username", "new_user_12345");
    ASSERT_TRUE(store.exists("username"));
    store.set("password", "password12345");

    std::cout << "size: " << store.size() << std::endl;
    store.print_data();

    EXPECT_TRUE(store.exists("password"));
    EXPECT_FALSE(store.exists("username"));
}

// Test that a Store with 2 cacpacity accepts exactly 2 pairings.
TEST(StoreLRUTest, CapacityTwoAcceptsTwoKeys) {
    Store store{2};
    store.set("car", "red");
    store.set("bike", "yellow");
    ASSERT_TRUE(store.exists("car"));
    ASSERT_TRUE(store.exists("bike"));

    store.get("car");
    store.set("boat", "blue");

    EXPECT_TRUE(store.exists("car"));
    EXPECT_TRUE(store.exists("boat"));
    EXPECT_FALSE(store.exists("bike"));
}

// Test that the Store never has more than the current capacity.
TEST(StoreLRUTest, StoreNeverExceedsCapacity) {
    Store store{3};

    for (int i = 0; i < 100; ++i) {
        store.set("key" + std::to_string(i), "value" + std::to_string(i));

        EXPECT_LE(store.size(), 3);
    }

    EXPECT_EQ(store.size(), 3);
}

// Test that a Store with no capacity is not limited.
TEST(StoreLRUTest, DefaultStorePreservesUnlimitedBehaviour) {
    Store store{};
    int size = 1000;

    for (int i = 0; i < size; ++i) {
        store.set("key" + std::to_string(i), "value" + std::to_string(i));
    }

    EXPECT_EQ(store.size(), size);
}

// Test that the oldest key is deleted.
TEST(StoreLRUTest, OldestKeyEvicted) {
    Store store{2};
    store.set("1", "1");
    store.set("2", "2");
    store.set("3", "3");

    EXPECT_FALSE(store.exists("1"));
    EXPECT_TRUE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
}

// Test that getting a missing key does not change the LRU order.
TEST(StoreLRUTest, GetMissingDoesNotChangeLRU) {
    Store store{2};
    store.set("1", "1");
    store.set("2", "2");
    store.set("3", "3");
    store.get("4");

    EXPECT_FALSE(store.exists("1"));
    EXPECT_TRUE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
}

// Test that get makes a key newer in the LRU order.
TEST(StoreLRUTest, GetMakesRecent) {
    Store store{2};
    store.set("1", "1");
    store.set("2", "2");
    store.get("1");
    store.set("3", "3");

    EXPECT_TRUE(store.exists("1"));
    EXPECT_FALSE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
}

// Test that the constantly getting makes a key stay in the data.
TEST(StoreLRUTest, RepeatedGetsKeepLRU) {
    Store store{2};
    store.set("key", "value");
    for (int i = 0; i < 100; ++i) {
        store.set("key" + std::to_string(i), "value" + std::to_string(i));
        store.get("key");
    }

    EXPECT_TRUE(store.exists("key"));
}

// Test that set (replacement) makes a key newer in the LRU order.
TEST(StoreLRUTest, SetMakesRecent) {
    Store store{2};
    store.set("1", "1");
    store.set("2", "2");
    store.set("1", "new 1");
    store.set("3", "3");

    store.print_data();

    EXPECT_TRUE(store.exists("1"));
    EXPECT_FALSE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
}

// Test that repeated insertions evict keys in the correct LRU order.
TEST(StoreLRUTest, MultipleEvictionsMaintainCorrectOrder) {
    Store store{2};

    store.set("1", "one");
    store.set("2", "two");

    store.set("3", "three");

    EXPECT_FALSE(store.exists("1"));
    EXPECT_TRUE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
    EXPECT_EQ(store.size(), 2);

    store.set("4", "four");

    EXPECT_FALSE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
    EXPECT_TRUE(store.exists("4"));
    EXPECT_EQ(store.size(), 2);

    store.set("5", "five");

    EXPECT_FALSE(store.exists("3"));
    EXPECT_TRUE(store.exists("4"));
    EXPECT_TRUE(store.exists("5"));
    EXPECT_EQ(store.size(), 2);
}

// Test that replacement does not change capacity.
TEST(StoreLRUTest, ReplacingKeyDoesNotIncreaseSize) {
    Store store{2};
    store.set("1", "1");
    store.set("2", "2");

    ASSERT_TRUE(store.size() == 2);
    store.set("1", "new 1");
    EXPECT_TRUE(store.size() == 2);
}

// Test that replacement does not evict.
TEST(StoreLRUTest, ReplacingKeyDoesNotEvict) {
    Store store{2};
    store.set("1", "1");
    store.set("2", "2");

    ASSERT_TRUE(store.size() == 2);
    store.set("1", "new 1");
    EXPECT_TRUE(store.exists("1"));
    EXPECT_TRUE(store.exists("2"));

    auto new_value = store.get("1");
    ASSERT_TRUE(new_value.has_value());
    EXPECT_EQ(new_value.value(), "new 1");
}

// Test that replacement removes expiration.
TEST(StoreLRUTest, ReplacingKeyReplacesExpiration) {
    Store store{2};
    store.set("1", "1");

    Store::ExpireResult result = store.expire("1", std::chrono::milliseconds{50});
    store.set("1", "1");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(store.exists("1"));
}

// Test that deleting frees capacity.
TEST(StoreLRUTest, DeletingKeyFreesCapacity) {
    Store store{3};
    store.set("1", "1");
    store.set("2", "2");
    store.set("3", "3");
    store.remove("2");
    store.set("4", "4");

    EXPECT_TRUE(store.exists("1"));
    EXPECT_TRUE(store.exists("3"));
    EXPECT_TRUE(store.exists("4"));
}

// Test that deleting a key also removes it from LRU tracking.
TEST(StoreLRUTest, DeletedKeyIsRemovedFromLruTracking) {
    Store store{2};

    store.set("1", "one");
    store.set("2", "two");

    ASSERT_TRUE(store.remove("1"));

    store.set("3", "three");
    store.set("4", "four");

    EXPECT_EQ(store.size(), 2);
    EXPECT_FALSE(store.exists("1"));
    EXPECT_FALSE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
    EXPECT_TRUE(store.exists("4"));
}

// Test that failed deletion adoes not affect LRU tracking.
TEST(StoreLRUTest, FailedDeleteDoesNotAffectLRU) {
    Store store{2};

    store.set("1", "one");
    store.set("2", "two");

    ASSERT_FALSE(store.remove("9"));

    store.set("3", "three");
    store.set("4", "four");

    EXPECT_EQ(store.size(), 2);
    EXPECT_FALSE(store.exists("1"));
    EXPECT_FALSE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
    EXPECT_TRUE(store.exists("4"));
}

// Test that exists does not update LRU order.
TEST(StoreLRUTest, ExistsDoesNotAffectLRU) {
    Store store{3};
    store.set("1", "1");
    store.set("2", "2");
    store.set("3", "3");
    store.exists("1");
    store.set("4", "4");

    EXPECT_TRUE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
    EXPECT_TRUE(store.exists("4"));
}

// Test that expire does not update LRU order.
TEST(StoreLRUTest, ExpireDoesNotAffectLRU) {
    Store store{3};
    store.set("1", "1");
    store.set("2", "2");
    store.set("3", "3");
    store.expire("1", std::chrono::milliseconds{100});
    store.set("4", "4");

    EXPECT_TRUE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
    EXPECT_TRUE(store.exists("4"));
}

// Test that expired keys are not tracked by LRU.
TEST(StoreLRUTest, ExpiredDoesNotAffectLRU) {
    Store store{3};
    store.set("1", "1");
    store.set("2", "2");
    store.set("3", "3");
    store.expire("3", std::chrono::milliseconds{1});
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    store.set("4", "4");

    EXPECT_TRUE(store.exists("1"));
    EXPECT_TRUE(store.exists("2"));
    EXPECT_FALSE(store.exists("3"));
    EXPECT_TRUE(store.exists("4"));
}

// Test that setting expired keys become the LRU.
TEST(StoreLRUTest, SettingExpiredMakesLRU) {
    Store store{2};
    store.set("1", "1");
    store.set("2", "2");
    store.expire("1", std::chrono::milliseconds{1});
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    store.set("1", "new 1");
    store.set("3", "3");

    EXPECT_TRUE(store.exists("1"));
    EXPECT_FALSE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
}

// Test that removing many expired entries does not desynchronize the map and LRU list.
TEST(StoreLRUTest, MultipleExpiredEntriesMaintainValidLruTracking) {
    Store store{4};

    store.set("1", "1");
    store.set("2", "2");
    store.set("3", "3");
    store.set("4", "4");

    ASSERT_EQ(store.expire("2", std::chrono::milliseconds{1}), Store::ExpireResult::SUCCESS);
    ASSERT_EQ(store.expire("4", std::chrono::milliseconds{1}), Store::ExpireResult::SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds{3});


    store.set("5", "five");

    EXPECT_EQ(store.size(), 3);
    EXPECT_TRUE(store.exists("1"));
    EXPECT_FALSE(store.exists("2"));
    EXPECT_TRUE(store.exists("3"));
    EXPECT_FALSE(store.exists("4"));
    EXPECT_TRUE(store.exists("5"));

    store.set("6", "six");
    store.set("7", "seven");

    // expect to evict key 1
    EXPECT_EQ(store.size(), 4);
    EXPECT_FALSE(store.exists("1"));
    EXPECT_TRUE(store.exists("3"));
    EXPECT_TRUE(store.exists("5"));
    EXPECT_TRUE(store.exists("6"));
    EXPECT_TRUE(store.exists("7"));

    ASSERT_TRUE(store.remove("5"));
    store.set("8", "eight");
    store.set("9", "nine");

    // expect to evict key 3
    EXPECT_EQ(store.size(), 4);
    EXPECT_FALSE(store.exists("3"));
    EXPECT_FALSE(store.exists("5"));
    EXPECT_TRUE(store.exists("6"));
    EXPECT_TRUE(store.exists("7"));
    EXPECT_TRUE(store.exists("8"));
    EXPECT_TRUE(store.exists("9"));
}

// Test that many parallel set operations do not corrupt the store.
TEST(StoreLRUTest, ConcurrentSetsDoNotCorruptStore) {
    constexpr int thread_count = 8;
    constexpr int keys_per_thread = 100;
    constexpr int total_keys = thread_count * keys_per_thread;

    Store store{static_cast<std::size_t>(total_keys)};
    std::barrier<> start_barrier{thread_count};
    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    // create threads, each waits for each other thread to start dumping keys
    for (int thread_id = 0; thread_id < thread_count; ++thread_id) {
        threads.emplace_back([&store, &start_barrier, thread_id]() {
            start_barrier.arrive_and_wait();

            for (int key_id = 0; key_id < keys_per_thread; ++key_id) {
                std::string key = "thread-" + std::to_string(thread_id) + "-key-" + std::to_string(key_id);
                std::string value = "value-" + std::to_string(thread_id) + "-" + std::to_string(key_id);
                store.set(key, value);
            }
        });
    }

    // recombine threads
    for (std::thread& thread : threads) {
        thread.join();
    }

    // verify threads
    ASSERT_EQ(store.size(), static_cast<std::size_t>(total_keys));
    for (int thread_id = 0; thread_id < thread_count; ++thread_id) {
        for (int key_id = 0; key_id < keys_per_thread; ++key_id) {
            std::string key =
                "thread-" + std::to_string(thread_id) +
                "-key-" + std::to_string(key_id);

            std::string expected_value =
                "value-" + std::to_string(thread_id) +
                "-" + std::to_string(key_id);

            auto actual_value = store.get(key);

            ASSERT_TRUE(actual_value.has_value());
            EXPECT_EQ(actual_value.value(), expected_value);
        }
    }
}

// Test that many parallel set operations do not surpass the Store's capacity.
TEST(StoreLRUTest, ConcurrentSetsDoNotSurpassCapacity) {
    constexpr int thread_count = 8;
    constexpr int keys_per_thread = 100;
    constexpr int total_keys = (thread_count * keys_per_thread) / 2;

    Store store{static_cast<std::size_t>(total_keys)};
    std::barrier<> start_barrier{thread_count};
    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    // create threads, each waits for each other thread to start dumping keys
    for (int thread_id = 0; thread_id < thread_count; ++thread_id) {
        threads.emplace_back([&store, &start_barrier, thread_id]() {
            start_barrier.arrive_and_wait();

            for (int key_id = 0; key_id < keys_per_thread; ++key_id) {
                std::string key = "thread-" + std::to_string(thread_id) + "-key-" + std::to_string(key_id);
                std::string value = "value-" + std::to_string(thread_id) + "-" + std::to_string(key_id);
                store.set(key, value);
            }
        });
    }

    // recombine threads
    for (std::thread& thread : threads) {
        thread.join();
    }

    // verify capacity
    ASSERT_EQ(store.size(), total_keys);
}

// Test that many parallel gets and sets leave the store valid
TEST(StoreLRUTest, ConcurrentGetsAndSetsPreserveValidEntries) {
    constexpr int thread_count = 8;
    constexpr int operations_per_thread = 200;

    Store store{8};

    for (int i = 0; i < 4; ++i) {
        store.set("hot-" + std::to_string(i), "initial");
    }

    std::barrier<> start_barrier{thread_count};
    std::atomic<bool> invalid_value_found{false};
    std::vector<std::thread> threads;

    for (int thread_id = 0; thread_id < thread_count; ++thread_id) {
        threads.emplace_back([&store, &start_barrier, &invalid_value_found, thread_id]() {
                start_barrier.arrive_and_wait();

                for (int i = 0; i < operations_per_thread; ++i) {
                    std::string hot_key = "hot-" + std::to_string(i % 4);

                    // first 4 threads reset the hoy key
                    if (thread_id < 4) {
                        // thread constantly replaces the hot key
                        std::string value ="version-" + std::to_string(i % 2);
                        store.set(hot_key, value);

                        // add some noise
                        store.set("noise-" + std::to_string(thread_id) + "-" + std::to_string(i), "noise");
                    } 
                    // last 4 threads get the hot key and search for the hot key still being in the store
                    else {
                        auto value = store.get(hot_key);
                        // hot key should always be one of these known values
                        if (value.has_value() &&
                            value.value() != "initial" &&
                            value.value() != "version-0" &&
                            value.value() != "version-1") {
                            invalid_value_found = true;
                        }
                    }
                }
            }
        );
    }

    for (std::thread& thread : threads) {
        thread.join();
    }

    // the hot key should never be a corrupted value
    EXPECT_FALSE(invalid_value_found.load());
    EXPECT_LE(store.size(), 8);

    // check the store is still functional
    for (int i = 0; i < 8; ++i) {
        store.set(
            "final-" + std::to_string(i),
            "value-" + std::to_string(i)
        );
    }

    EXPECT_EQ(store.size(), 8);

    for (int i = 0; i < 8; ++i) {
        EXPECT_TRUE(store.exists("final-" + std::to_string(i)));
    }
}

// Test that concurrent deletes and sets do not corrupt the store
TEST(StoreLRUTest, ConcurrentDeleteAndSetDoesNotCorruptLruTracking) {
    constexpr int thread_count = 8;
    constexpr int operations_per_thread = 300;

    Store store{16};

    for (int i = 0; i < 16; ++i) {
        store.set("key-" + std::to_string(i), "initial");
    }

    std::barrier<> start_barrier{thread_count};
    std::vector<std::thread> threads;

    // thread alternate between adding and removing various keys
    for (int thread_id = 0; thread_id < thread_count; ++thread_id) {
        threads.emplace_back([&store, &start_barrier, thread_id]() {
                start_barrier.arrive_and_wait();
                for (int i = 0; i < operations_per_thread; ++i) {
                    std::string key = "key-" + std::to_string((thread_id * operations_per_thread + i) % 32);

                    if (thread_id % 2 == 0) {
                        store.set(key, "updated");
                    } else {
                        store.remove(key);
                    }
                }
            }
        );
    }

    for (std::thread& thread : threads) {
        thread.join();
    }
    EXPECT_LE(store.size(), 16);

    // remove every key and check that the store still works
    for (int i = 0; i < 32; ++i) {
        store.remove("key-" + std::to_string(i));
    }
    ASSERT_EQ(store.size(), 0);

    for (int i = 0; i < 16; ++i) {
        store.set(
            "final-" + std::to_string(i),
            "value"
        );
    }

    store.set("final-16", "value");
    EXPECT_EQ(store.size(), 16);
    EXPECT_FALSE(store.exists("final-0"));

    for (int i = 1; i <= 16; ++i) {
        EXPECT_TRUE(store.exists("final-" + std::to_string(i)));
    }
}