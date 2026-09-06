#include <gtest/gtest.h>

#include "cache_server/store.hpp"
#include "cache_server/command_processor.hpp"

// Test that processor can process set properly.
TEST(ProcessorTest, ProccessesSetCorrectly) {
    Store store;
    CommandProcessor processor{store};

    std::string parsed_command = processor.execute(Command {CommandType::SET, "username", "user_name_12345", std::nullopt});
    ASSERT_EQ(parsed_command, "OK");
    EXPECT_EQ(store.get("username").value(), "user_name_12345");
}

// Test that processor can process a get properly when the key is present.
TEST(ProcessorTest, ProccessesGetCorrectlyKeyPresent) {
    Store store;
    store.set("username", "user_name_12345");
    CommandProcessor processor{store};

    std::string parsed_command = processor.execute(Command {CommandType::GET, "username", std::nullopt, std::nullopt});
    ASSERT_EQ(parsed_command, "VALUE user_name_12345");
    EXPECT_EQ(store.get("username").value(), "user_name_12345");
}

// Test that processor can process a correct get properly when a key is not present.
TEST(ProcessorTest, ProccessesGetCorrectlyKeyMissing) {
    Store store;
    CommandProcessor processor{store};

    std::string parsed_command = processor.execute(Command {CommandType::GET, "username", std::nullopt, std::nullopt});
    ASSERT_EQ(parsed_command, "NOT_FOUND");
}

// Test that processor can process a delete properly when the key is present.
TEST(ProcessorTest, ProccessesDeleteCorrectlyKeyPresent) {
    Store store;
    store.set("username", "user_name_12345");
    CommandProcessor processor{store};

    std::string parsed_command = processor.execute(Command {CommandType::DELETE, "username", std::nullopt, std::nullopt});
    ASSERT_EQ(parsed_command, "INTEGER 1");
    EXPECT_FALSE(store.exists("username"));
}

// Test that processor can process a delete properly when a key is not present.
TEST(ProcessorTest, ProccessesDeleteCorrectlyKeyMissing) {
    Store store;
    CommandProcessor processor{store};

    std::string parsed_command = processor.execute(Command {CommandType::DELETE, "username", std::nullopt, std::nullopt});
    EXPECT_EQ(parsed_command, "INTEGER 0");
}

// Test that processor can process an exists properly when the key is present.
TEST(ProcessorTest, ProccessesExistsCorrectlyKeyPresent) {
    Store store;
    store.set("username", "user_name_12345");
    CommandProcessor processor{store};

    std::string parsed_command = processor.execute(Command {CommandType::EXISTS, "username", std::nullopt, std::nullopt});
    ASSERT_EQ(parsed_command, "INTEGER 1");
    EXPECT_TRUE(store.exists("username"));
}

// Test that processor can process an exists properly when a key is not present.
TEST(ProcessorTest, ProccessesExistsCorrectlyKeyMissing) {
    Store store;
    CommandProcessor processor{store};

    std::string parsed_command = processor.execute(Command {CommandType::EXISTS, "username", std::nullopt, std::nullopt});
    EXPECT_EQ(parsed_command, "INTEGER 0");
}

// Test that processor can process an expire properly when the key is present.
TEST(ProcessorTest, ProccessesExpiresCorrectlyKeyPresent) {
    Store store;
    store.set("username", "user_name_12345");
    CommandProcessor processor{store};

    std::string parsed_command = processor.execute(Command {CommandType::EXPIRE, "username", std::nullopt, std::chrono::seconds{1}});
    ASSERT_EQ(parsed_command, "INTEGER 1");
    EXPECT_TRUE(store.exists("username"));
}

// Test that processor can process an expires properly when a key is not present.
TEST(ProcessorTest, ProccessesExpiresCorrectlyKeyMissing) {
    Store store;
    CommandProcessor processor{store};

    std::string parsed_command = processor.execute(Command {CommandType::EXPIRE, "username", std::nullopt, std::chrono::seconds{1}});
    EXPECT_EQ(parsed_command, "INTEGER 0");
}

// Test that processor can process an expires properly when an invalid duration.
TEST(ProcessorTest, ProccessesExpiresCorrectlyInvalidDuration) {
    Store store;
    store.set("username", "user_name_12345");
    CommandProcessor processor{store};

    std::string parsed_command = processor.execute(Command {CommandType::EXPIRE, "username", std::nullopt, std::chrono::seconds{0}});
    EXPECT_EQ(parsed_command, "ERROR invalid duration");
}

// Test that processor can process a series of commands successfully.
TEST(ProcessorTest, ProccessesSeriesCorrectly) {
    Store store;
    store.set("username", "user_name_12345");
    CommandProcessor processor{store};

    std::string parsed_command_1 = processor.execute(Command {CommandType::SET, "car", "red", std::nullopt});
    std::string parsed_command_2 = processor.execute(Command {CommandType::SET, "bike", "blue", std::nullopt});
    std::string parsed_command_3 = processor.execute(Command {CommandType::GET, "car", std::nullopt, std::nullopt});
    std::string parsed_command_4 = processor.execute(Command {CommandType::DELETE, "car", std::nullopt, std::nullopt});
    std::string parsed_command_5 = processor.execute(Command {CommandType::GET, "car", std::nullopt, std::nullopt});
    std::string parsed_command_6 = processor.execute(Command {CommandType::SET, "car", "green", std::nullopt});
    EXPECT_EQ(parsed_command_1, "OK");
    EXPECT_EQ(parsed_command_2, "OK");
    EXPECT_EQ(parsed_command_3, "VALUE red");
    EXPECT_EQ(parsed_command_4, "INTEGER 1");
    EXPECT_EQ(parsed_command_5, "NOT_FOUND");
    EXPECT_EQ(parsed_command_6, "OK");
    EXPECT_EQ(store.get("car").value(), "green");
}

// Test that two processors with the same store affect each other.
TEST(ProcessorTest, ProccessesMultipleProcessorsCorrectly) {
    Store store;
    store.set("username", "user_name_12345");
    CommandProcessor processor_1{store};
    CommandProcessor processor_2{store};

    std::string parsed_command_1 = processor_1.execute(Command {CommandType::SET, "car", "red", std::nullopt});
    std::string parsed_command_2 = processor_2.execute(Command {CommandType::SET, "bike", "blue", std::nullopt});
    std::string parsed_command_3 = processor_1.execute(Command {CommandType::GET, "bike", std::nullopt, std::nullopt});
    std::string parsed_command_4 = processor_2.execute(Command {CommandType::GET, "car", std::nullopt, std::nullopt});
    EXPECT_EQ(parsed_command_1, "OK");
    EXPECT_EQ(parsed_command_2, "OK");
    EXPECT_EQ(parsed_command_3, "VALUE blue");
    EXPECT_EQ(parsed_command_4, "VALUE red");
    EXPECT_EQ(store.get("car").value(), "red");
    EXPECT_EQ(store.get("bike").value(), "blue");
}

// Test that we process a TTL command with missing key correctly
TEST(ProcessorTest, ProcessesTtlMissingKey) {
    Store store;
    CommandProcessor processor{store};

    std::string response = processor.execute(Command{CommandType::TTL, "username", std::nullopt, std::nullopt});
    EXPECT_EQ(response, "INTEGER -2");
}

// Test that we process a TTL command with missing key correctly
TEST(ProcessorTest, ProcessesTtlNotExpired) {
    Store store;
    store.set("car","red");
    CommandProcessor processor{store};

    std::string response = processor.execute(Command{CommandType::TTL, "car", std::nullopt, std::nullopt});
    EXPECT_EQ(response, "INTEGER -1");
}

// Test that we process a TTL command with a soon to expire key
TEST(ProcessorTest, ProcessesTtlHasExpiry) {
    Store store;
    store.set("car","red");
    store.expire("car", std::chrono::seconds{10});
    CommandProcessor processor{store};

    std::string response = processor.execute(Command{CommandType::TTL, "car", std::nullopt, std::nullopt});
    EXPECT_TRUE(response == "INTEGER 9" || response == "INTEGER 10");
}

// Test that we process a STATS command with a soon to expire key
TEST(ProcessorTest, ProcessesStatsCorrectly) {
    Store store{2};
    store.set("car","red");
    store.set("bike", "blue");
    store.get("car");
    store.get("missing");
    store.set("boat", "green");

    CommandProcessor processor{store};
    std::string response = processor.execute(Command{CommandType::STATS, "", std::nullopt, std::nullopt});

    EXPECT_EQ(response,
              "STATS entries=2 capacity=2 hits=1 misses=1 "
              "evictions=1 expirations=0");
}