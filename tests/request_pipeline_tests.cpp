#include <gtest/gtest.h>

#include "cache_server/store.hpp"
#include "cache_server/command_processor.hpp"
#include "cache_server/request_parser.hpp"

// Test that processor/parser can process a sequence properly.
TEST(RequestPipelineTest, ProcessesSetGetDeleteSequence) {
    Store store;
    CommandProcessor processor{store};
    RequestParser parser;
    ParseResult command_1 = parser.parse("SET username user_name_12345");
    ParseResult command_2 = parser.parse("GET username");
    ParseResult command_3 = parser.parse("EXISTS username");
    ParseResult command_4 = parser.parse("DELETE username");
    ParseResult command_5 = parser.parse("GET username");

    ASSERT_TRUE(std::holds_alternative<Command>(command_1));
    ASSERT_TRUE(std::holds_alternative<Command>(command_2));
    ASSERT_TRUE(std::holds_alternative<Command>(command_3));
    ASSERT_TRUE(std::holds_alternative<Command>(command_4));
    ASSERT_TRUE(std::holds_alternative<Command>(command_5));

    std::string parsed_command_1 = processor.execute(std::get<Command>(command_1));
    std::string parsed_command_2 = processor.execute(std::get<Command>(command_2));
    std::string parsed_command_3 = processor.execute(std::get<Command>(command_3));
    std::string parsed_command_4 = processor.execute(std::get<Command>(command_4));
    std::string parsed_command_5 = processor.execute(std::get<Command>(command_5));

    EXPECT_EQ(parsed_command_1, "OK");
    EXPECT_EQ(parsed_command_2, "VALUE user_name_12345");
    EXPECT_EQ(parsed_command_3, "INTEGER 1");
    EXPECT_EQ(parsed_command_4, "INTEGER 1");
    EXPECT_EQ(parsed_command_5, "NOT_FOUND");
}

// TTest that the complete pipeline can handle expire properly.
TEST(RequestPipelineTest, PipelineProccessesExpireCorrectly) {
    Store store;
    CommandProcessor processor{store};
    RequestParser parser;
    ParseResult command_1 = parser.parse("SET username user_name_12345");
    ParseResult command_2 = parser.parse("EXPIRE username 60");

    ASSERT_TRUE(std::holds_alternative<Command>(command_1));
    ASSERT_TRUE(std::holds_alternative<Command>(command_2));

    std::string parsed_command_1 = processor.execute(std::get<Command>(command_1));
    std::string parsed_command_2 = processor.execute(std::get<Command>(command_2));
    EXPECT_EQ(parsed_command_1, "OK");
    EXPECT_EQ(parsed_command_2, "INTEGER 1");
}

// Test that the complete pipeline can handle ttl properly.
TEST(RequestPipelineTest, PipelineProccessesTtlCorrectly) {
    Store store;
    CommandProcessor processor{store};
    RequestParser parser;
    ParseResult command_1 = parser.parse("SET username user_name_12345");
    ParseResult command_2 = parser.parse("EXPIRE username 60");
    ParseResult command_3 = parser.parse("TTL username");

    ASSERT_TRUE(std::holds_alternative<Command>(command_1));
    ASSERT_TRUE(std::holds_alternative<Command>(command_2));
    ASSERT_TRUE(std::holds_alternative<Command>(command_3));

    std::string parsed_command_1 = processor.execute(std::get<Command>(command_1));
    std::string parsed_command_2 = processor.execute(std::get<Command>(command_2));
    std::string parsed_command_3 = processor.execute(std::get<Command>(command_3));
    EXPECT_EQ(parsed_command_1, "OK");
    EXPECT_EQ(parsed_command_2, "INTEGER 1");
    EXPECT_TRUE(parsed_command_3 == "INTEGER 60" || parsed_command_3 == "INTEGER 59");
}

// Test that the complete pipeline can handle stats properly.
TEST(RequestPipelineTest, PipelineProccessesStatsCorrectly) {
    Store store{2};
    CommandProcessor processor{store};
    RequestParser parser;
    ParseResult command_1 = parser.parse("SET car red");
    ParseResult command_2 = parser.parse("SET bike blue");
    ParseResult command_3 = parser.parse("GET car");
    ParseResult command_4 = parser.parse("GET missing");
    ParseResult command_5 = parser.parse("SET boat green");
    ParseResult command_6 = parser.parse("STATS");

    ASSERT_TRUE(std::holds_alternative<Command>(command_1));
    ASSERT_TRUE(std::holds_alternative<Command>(command_2));
    ASSERT_TRUE(std::holds_alternative<Command>(command_3));
    ASSERT_TRUE(std::holds_alternative<Command>(command_4));
    ASSERT_TRUE(std::holds_alternative<Command>(command_5));
    ASSERT_TRUE(std::holds_alternative<Command>(command_6));

    std::string parsed_command_1 = processor.execute(std::get<Command>(command_1));
    std::string parsed_command_2 = processor.execute(std::get<Command>(command_2));
    std::string parsed_command_3 = processor.execute(std::get<Command>(command_3));
    std::string parsed_command_4 = processor.execute(std::get<Command>(command_4));
    std::string parsed_command_5 = processor.execute(std::get<Command>(command_5));
    std::string parsed_command_6 = processor.execute(std::get<Command>(command_6));

    EXPECT_EQ(parsed_command_1, "OK");
    EXPECT_EQ(parsed_command_2, "OK");
    EXPECT_EQ(parsed_command_3, "VALUE red");
    EXPECT_EQ(parsed_command_4, "NOT_FOUND");
    EXPECT_EQ(parsed_command_5, "OK");
    EXPECT_EQ(parsed_command_6,
             "STATS entries=2 "
             "capacity=2 "
             "hits=1 "
             "misses=1 "
             "evictions=1 "
             "expirations=0"
    );
}

// Test that the complete pipeline can handle parse error properly.
TEST(RequestPipelineTest, PipelineProccessesUnkownCommandCorrectly) {
    Store store;
    CommandProcessor processor{store};
    RequestParser parser;
    ParseResult command = parser.parse("UNKNOWN");

    EXPECT_TRUE(std::holds_alternative<ParseError>(command));
    EXPECT_EQ(std::get<ParseError>(command), ParseError::UNKNOWN_COMMAND);
}

// Test that the complete pipeline can handle lowercase.
TEST(RequestPipelineTest, PipelineProccessesLowercaseCorrectly) {
    Store store;
    CommandProcessor processor{store};
    RequestParser parser;
    ParseResult command = parser.parse("set username user_name_12345");

    ASSERT_TRUE(std::holds_alternative<Command>(command));
    std::string parsed_command = processor.execute(std::get<Command>(command));
    EXPECT_EQ(parsed_command, "OK");
}

