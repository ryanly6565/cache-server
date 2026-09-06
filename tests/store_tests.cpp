#include <gtest/gtest.h>

#include "cache_server/store.hpp"

// Test that new Store is empty.
TEST(StoreTest, NewStoreDoesNotContainKey) {
    Store store;

    EXPECT_FALSE(store.exists("username"));
}

// Test that retrieving from a new Store results in nothing.
TEST(StoreTest, NewStoreTestReturnsNothingOnGet) {
    Store store;

    EXPECT_EQ(store.get("username"), std::nullopt);
}

// Test that deleting in a new Store results in False.
TEST(StoreTest, NewStoreRemoveReturnsFalse) {
    Store store;

    EXPECT_FALSE(store.remove("username"));
}

// Test that setting a key causes its value to exist.
TEST(StoreTest, SettingCausesExistingKey) {
    Store store;
    store.set("username", "new_user_12345");

    EXPECT_TRUE(store.exists("username"));
}

// Test that setting a key allows its value to be retrieved.
TEST(StoreTest, SettingCausesRetrievableValue) {
    Store store;
    store.set("username", "new_user_12345");
    auto expected = store.get("username");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "new_user_12345");
}

// Test that setting two keys allow both to exist.
TEST(StoreTest, SettingTwoLetsBothExist) {
    Store store;
    store.set("username", "new_user_12345");
    store.set("password", "p@s5w0rD");

    EXPECT_TRUE(store.exists("username"));
    EXPECT_TRUE(store.exists("password"));
}

// Test that setting two keys allow both to be retrieved.
TEST(StoreTest, SettingTwoLetsBothBeRetrieved) {
    Store store;
    store.set("username", "new_user_12345");
    store.set("password", "p@s5w0rD");
    auto expected1 = store.get("username");
    auto expected2 = store.get("password");

    ASSERT_TRUE(expected1.has_value());
    ASSERT_TRUE(expected2.has_value());
    EXPECT_EQ(expected1.value(), "new_user_12345");
    EXPECT_EQ(expected2.value(), "p@s5w0rD");
}

// Test that setting a key again replaces its expected.
TEST(StoreTest, SettingAgainReplaces) {
    Store store;
    store.set("username", "new_user_12345");
    store.set("username", "guest");
    auto expected = store.get("username");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "guest");
}

// Test that replacing one key does not affect another.
TEST(StoreTest, SettingAgainDoesNotAffectOthers) {
    Store store;
    store.set("username", "new_user_12345");
    store.set("password", "p@s5w0rD");
    store.set("username", "guest");
    auto expected = store.get("password");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "p@s5w0rD");
}

// Test that setting a key's value twice does not change it.
TEST(StoreTest, SettingAgainWithSameDoesNotChange) {
    Store store;
    store.set("username", "new_user_12345");
    store.set("username", "new_user_12345");
    auto expected = store.get("username");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "new_user_12345");
}

// Test that getting a missing key result in an optional with no expected.
TEST(StoreTest, MissingKeyHasNoValue) {
    Store store;
    auto expected = store.get("username");

    ASSERT_FALSE(expected.has_value());
}

// Test that getting a key multiple does not affect the expected.
TEST(StoreTest, GettingRepeatedlyDoesNotChangeOriginal) {
    Store store;
    store.set("username", "new_user_12345");
    auto expected = store.get("username");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "new_user_12345");

    expected = store.get("username");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "new_user_12345");

    expected = store.get("username");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "new_user_12345");

    expected = store.get("username");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "new_user_12345");
}

// Test that removing an existing key returns true and deletes the expected.
TEST(StoreTest, RemovedKeyHasNoValue) {
    Store store;
    store.set("username", "new_user_12345");
    bool removed = store.remove("username");

    ASSERT_TRUE(removed);
    EXPECT_FALSE(store.exists("username"));
    EXPECT_EQ(store.get("username"), std::nullopt);
}

// Test that removing a non-existant key results in false.
TEST(StoreTest, RemovedKeyReturnsFalseOnRemoval) {
    Store store;

    EXPECT_FALSE(store.remove("username"));
}

// Test that removing a key twice returns true then false.
TEST(StoreTest, RemovingKeyTwiceReturnsTrueThenFalse) {
    Store store;
    store.set("username", "new_user_12345");
    bool removed = store.remove("username");

    ASSERT_TRUE(removed);
    EXPECT_FALSE(store.exists("username"));
    EXPECT_EQ(store.get("username"), std::nullopt);

    removed = store.remove("username");

    EXPECT_FALSE(removed);
}

// Test that removing one key does not remove others.
TEST(StoreTest, RemovingDoesNotAffectOthers) {
    Store store;
    store.set("username", "new_user_12345");
    store.set("password", "p@s5w0rD");
    store.remove("username");
    auto expected = store.get("password");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "p@s5w0rD");
}

// Test that a removed key can be set again.
TEST(StoreTest, RemovingAllowsResetting) {
    Store store;
    store.set("username", "new_user_12345");
    store.remove("username");
    store.set("username", "guest");
    auto expected = store.get("username");

    ASSERT_TRUE(store.exists("username"));
    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "guest");
}

// Test that exists fails for a non-existent key.
TEST(StoreTest, ExistsFailsForNonExistent) {
    Store store;

    EXPECT_FALSE(store.exists("username"));
}

// Test that calling exists repeatedly returns the same result.
TEST(StoreTest, ExistsRepeatedlyWorks) {
    Store store;
    store.set("username", "new_user_12345");

    EXPECT_EQ(store.exists("username"), true);
    EXPECT_EQ(store.exists("username"), true);
    EXPECT_EQ(store.exists("username"), true);
    EXPECT_EQ(store.exists("username"), true);
    EXPECT_EQ(store.exists("username"), true);
}

// Test that keys are case-sensitive.
TEST(StoreTest, KeysAreCaseSensitive) {
    Store store;
    store.set("username", "new_user_12345");

    EXPECT_TRUE(store.exists("username"));
    EXPECT_FALSE(store.exists("Username"));
    EXPECT_FALSE(store.exists("userName"));
    EXPECT_FALSE(store.exists("UsernamE"));
}

// Test that value casing is preserved.
TEST(StoreTest, ValueCasingIsPreserved) {
    Store store;
    store.set("username", "New_User_12345");
    auto expected = store.get("username");

    ASSERT_TRUE(store.exists("username"));
    ASSERT_TRUE(expected.has_value());
    EXPECT_NE(expected.value(), "new_user_12345");
    EXPECT_EQ(expected.value(), "New_User_12345");
}

// Test that similar keys are treated differently.
TEST(StoreTest, SimilatKeysAreDifferent) {
    Store store;
    store.set("username", "new_user_12345");
    store.set("user", "guest");
    auto expected1 = store.get("username");
    auto expected2 = store.get("user");

    ASSERT_TRUE(expected1.has_value());
    ASSERT_TRUE(expected2.has_value());
    EXPECT_EQ(expected1.value(), "new_user_12345");
    EXPECT_EQ(expected2.value(), "guest");
}

// Test that the store can handle keys with spaces.
TEST(StoreTest, SpacingIsAllowed) {
    Store store;
    store.set("user name", "new user 12345");
    auto expected = store.get("user name");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "new user 12345");
}

// Test that the store can handle puncutation.
TEST(StoreTest, PunctuationIsAllowed) {
    Store store;
    store.set("user.name!!!\n", "!@#$%^&*()_+");
    auto expected = store.get("user.name!!!\n");

    ASSERT_TRUE(expected.has_value());
    EXPECT_EQ(expected.value(), "!@#$%^&*()_+");
}

// Test that the empty string can be stored.
TEST(StoreTest, EmptyStringWorks) {
    Store store;
    store.set("username", "");
    auto expected = store.get("username");

    ASSERT_TRUE(store.exists("username"));
    EXPECT_EQ(expected.value(), "");
}

// Test that long keys can be used.
TEST(StoreTest, LongKeyCanBeStoredAndRetrieved) {
    Store store;
    std::string long_key(10000, 'k');

    store.set(long_key, "new_user_12345");
    auto result = store.get(long_key);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "new_user_12345");
}

// Test that long values can be used.
TEST(StoreTest, LongValueCanBeStoredAndRetrieved) {
    Store store;
    std::string long_value(10000, 'v');

    store.set("username", long_value);
    auto result = store.get("username");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), long_value);
}

// Test that unicode values and keys work.
TEST(StoreTest, UnicodeKeyCanBeStoredAndRetrieved) {
    Store store;
    std::string unicode_key = "使用者名";
    std::string unicode_value = "αξία";

    store.set(unicode_key, unicode_value);
    auto result = store.get(unicode_key);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), unicode_value);
}

// Test that calling expire with an invalid duration fails.
TEST(StoreTest, ExpireInvalidDurationFails) {
    Store store;

    store.set("username", "new_user_12345");
    Store::ExpireResult result = store.expire("user2", std::chrono::seconds{-10});
    EXPECT_EQ(result, Store::ExpireResult::INVALID_DURATION);
    result = store.expire("user2", std::chrono::seconds{0});
    EXPECT_EQ(result, Store::ExpireResult::INVALID_DURATION);
}

// Test that calling expire on a missing key fails.
TEST(StoreTest, ExpireMissingFails) {
    Store store;

    store.set("username", "new_user_12345");
    Store::ExpireResult result = store.expire("user2", std::chrono::seconds{60});

    EXPECT_EQ(result, Store::ExpireResult::KEY_NOT_FOUND);
}

// Test that calling expire on valid arguments returns success.
TEST(StoreTest, ExpireSuccess) {
    Store store;

    store.set("username", "new_user_12345");
    Store::ExpireResult result = store.expire("username", std::chrono::seconds{60});

    EXPECT_EQ(result, Store::ExpireResult::SUCCESS);
}

// Test that expiry really deletes a key after an exists.
TEST(StoreTest, ExistsDeletesExpired) {
    Store store;

    store.set("username", "new_user_12345");
    Store::ExpireResult result = store.expire("username", std::chrono::milliseconds{1});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    EXPECT_FALSE(store.exists("username"));
}

// Test that expiry really deletes a key after a get.
TEST(StoreTest, GetDeletesExpired) {
    Store store;

    store.set("username", "new_user_12345");
    Store::ExpireResult result = store.expire("username", std::chrono::milliseconds{1});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    EXPECT_EQ(store.get("username"), std::nullopt);
}

// Test that expiry really deletes a key after a remove.
TEST(StoreTest, RemoveDeletesExpired) {
    Store store;

    store.set("username", "new_user_12345");
    Store::ExpireResult result = store.expire("username", std::chrono::milliseconds{1});
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    EXPECT_FALSE(store.remove("username"));
}

// Test that expiry really deletes a key after a get.
TEST(StoreTest, NoExpiryBeforeDeadline) {
    Store store;

    store.set("username", "new_user_12345");
    Store::ExpireResult result = store.expire("username", std::chrono::milliseconds{200});

    EXPECT_TRUE(store.exists("username"));
    EXPECT_EQ(store.get("username").value(), "new_user_12345");
    EXPECT_TRUE(store.remove("username"));
}

// Test that a Store withe cleanup interval 0 throws an error.
TEST(StoreTest, ZeroCleanupIntervalThrows) {
    EXPECT_THROW(Store store(4, std::chrono::milliseconds(0)), std::invalid_argument);
}

// Test that the cleanup thread is able to remove expired keys on its own.
TEST(StoreTest, ExpiredRemovedWithoutAccess) {
    Store store {1, std::chrono::milliseconds(5)};
    store.set("username", "new_user_12345");
    store.expire("username", std::chrono::milliseconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    ASSERT_EQ(store.size(), 0);
}

// Test that the keys not marked for expiry are not consumed by cleanup.
TEST(StoreTest, ExpirePreservesNonExpiry) {
    Store store {10, std::chrono::milliseconds(3)};
    store.set("car", "red");

    store.set("bike", "yellow");
    Store::ExpireResult result = store.expire("bike", std::chrono::milliseconds(1));
    EXPECT_EQ(result, Store::ExpireResult::SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
    ASSERT_FALSE(store.exists("bike"));
    ASSERT_EQ(store.size(), 1);
    
    store.set("boat", "blue");
    store.expire("boat", std::chrono::milliseconds(1));
    result = store.expire("bike", std::chrono::milliseconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(4));

    ASSERT_EQ(store.size(), 1);
    ASSERT_FALSE(store.exists("boat"));
    ASSERT_TRUE(store.exists("car"));
}

// Test that trying to expire an expired key does not revive it.
TEST(StoreTest, ExpiryLeavesExpired) {
    Store store {1, std::chrono::milliseconds(100)};
    store.set("username", "new_user_12345");
    store.expire("username", std::chrono::milliseconds(4));
    std::this_thread::sleep_for(std::chrono::milliseconds(6));
    ASSERT_EQ(store.expire("username", std::chrono::milliseconds(1)), Store::ExpireResult::KEY_NOT_FOUND);
    EXPECT_FALSE(store.exists("username"));
}

// Test that calling ttl on a missing key returns -2.
TEST(StoreTest, TtlMissingReturnsNegativeTwo) {
    Store store {1, std::chrono::milliseconds(100)};
    ASSERT_EQ(store.ttl("username"), -2);
}

// Test that calling ttl on a no-expire key returns -1.
TEST(StoreTest, TtlNoExpiryReturnsNegativeOne) {
    Store store {1, std::chrono::milliseconds(100)};
    store.set("username", "user_name_12345");
    ASSERT_EQ(store.ttl("username"), -1);
}

// Test that ttl returns proper time
TEST(StoreTest, TtlReturnsRemainingSeconds) {
    Store store {1, std::chrono::milliseconds(100)};
    store.set("username", "user_name_12345");
    store.expire("username", std::chrono::seconds(10));
    auto result = store.ttl("username");
    
    EXPECT_GE(result, 9);
    EXPECT_LE(result, 10);
}

// Test that ttl -2 on an already expired key
TEST(StoreTest, TtlExpiredReturnsNegativeTwo) {
    Store store {1, std::chrono::milliseconds(100)};
    store.set("username", "user_name_12345");
    store.expire("username", std::chrono::milliseconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(8));
    ASSERT_EQ(store.ttl("username"), -2);
}

