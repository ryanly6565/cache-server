#include <gtest/gtest.h>
#include "cache_server/request_parser.hpp"

// Test that an empty request returns an error.
TEST(RequestParserTest, ParsingEmptyGivesError) {
    RequestParser parser;
    auto result1 = parser.parse("");
    auto result2 = parser.parse("       ");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result1));
    ASSERT_TRUE(std::holds_alternative<ParseError>(result2));
    EXPECT_EQ((std::get<ParseError>(result1)), ParseError::EMPTY_REQUEST);
    EXPECT_EQ((std::get<ParseError>(result2)), ParseError::EMPTY_REQUEST);
}


// Test that an unknown command returns an error.
TEST(RequestParserTest, ParsingUnknownFirstTokenGivesError) {
    RequestParser parser;
    auto result = parser.parse("SETTING");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
    EXPECT_EQ((std::get<ParseError>(result)), ParseError::UNKNOWN_COMMAND);
}

// Test that a SET with no key returns an error
TEST(RequestParserTest, ParsingSetNoKey) {
    RequestParser parser;
    auto result = parser.parse("SET ");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
    EXPECT_EQ((std::get<ParseError>(result)), ParseError::EMPTY_KEY);
}

// Test that a SET with no value returns an error
TEST(RequestParserTest, ParsingSetNoValue) {
    RequestParser parser;
    auto result = parser.parse("SET username");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
    EXPECT_EQ((std::get<ParseError>(result)), ParseError::EMPTY_VALUE);
}

// Test that a GET with no key returns an error
TEST(RequestParserTest, ParsingGetNoKey) {
    RequestParser parser;
    auto result = parser.parse("GET");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
    EXPECT_EQ((std::get<ParseError>(result)), ParseError::EMPTY_KEY);
}

// Test that a DELETE with no key returns an error
TEST(RequestParserTest, ParsingDeleteNoKey) {
    RequestParser parser;
    auto result = parser.parse("DELETE");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
    EXPECT_EQ((std::get<ParseError>(result)), ParseError::EMPTY_KEY);
}

// Test that an EXISTS with no key returns an error
TEST(RequestParserTest, ParsingExistsNoKey) {
    RequestParser parser;
    auto result = parser.parse("EXISTS ");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
    EXPECT_EQ((std::get<ParseError>(result)), ParseError::EMPTY_KEY);
}

// Test that an EXPIRE with no key returns an error
TEST(RequestParserTest, ParsingExpireNoKey) {
    RequestParser parser;
    auto result = parser.parse("EXPIRE ");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
    EXPECT_EQ((std::get<ParseError>(result)), ParseError::EMPTY_KEY);
}

// Test that a command with too many arguments results in an error
TEST(RequestParserTest, ParsingRejectsTooManyArgs) {
    RequestParser parser;
    auto result1 = parser.parse("GET username user_name");
    auto result2 = parser.parse("DELETE username user_name");
    auto result3 = parser.parse("EXISTS username user_name");
    auto result4 = parser.parse("EXPIRE username 1 user_name");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result1));
    ASSERT_TRUE(std::holds_alternative<ParseError>(result2));
    ASSERT_TRUE(std::holds_alternative<ParseError>(result3));
    ASSERT_TRUE(std::holds_alternative<ParseError>(result4));
    EXPECT_EQ((std::get<ParseError>(result1)), ParseError::WRONG_ARGUMENT_COUNT);
    EXPECT_EQ((std::get<ParseError>(result2)), ParseError::WRONG_ARGUMENT_COUNT);
    EXPECT_EQ((std::get<ParseError>(result3)), ParseError::WRONG_ARGUMENT_COUNT);
    EXPECT_EQ((std::get<ParseError>(result4)), ParseError::WRONG_ARGUMENT_COUNT);
}

// Test that a valid set command works.
TEST(RequestParserTest, ParserParsesSet) {
    RequestParser parser;
    auto result = parser.parse("SET username user_name");

    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ((std::get<Command>(result)), (Command{CommandType::SET, "username", "user_name", std::nullopt}));
}

// Test that a command works with leading white space.
TEST(RequestParserTest, ParserParsesIgnoringWhiteSpace) {
    RequestParser parser;
    auto result1 = parser.parse("   \n SET    username     user_name");
    auto result2 = parser.parse("   \n GET    username   ");
    auto result3 = parser.parse("   \n DELETE    username   ");
    auto result4 = parser.parse("   \n EXISTS    username   ");
    auto result5 = parser.parse("   \n EXPIRE    username  12 ");

    ASSERT_TRUE(std::holds_alternative<Command>(result1));
    ASSERT_TRUE(std::holds_alternative<Command>(result2));
    ASSERT_TRUE(std::holds_alternative<Command>(result3));
    ASSERT_TRUE(std::holds_alternative<Command>(result4));
    ASSERT_TRUE(std::holds_alternative<Command>(result5));
    EXPECT_EQ((std::get<Command>(result1)), (Command{CommandType::SET, "username", "user_name", std::nullopt}));
    EXPECT_EQ((std::get<Command>(result2)), (Command{CommandType::GET, "username", std::nullopt, std::nullopt}));
    EXPECT_EQ((std::get<Command>(result3)), (Command{CommandType::DELETE, "username", std::nullopt, std::nullopt}));
    EXPECT_EQ((std::get<Command>(result4)), (Command{CommandType::EXISTS, "username", std::nullopt, std::nullopt}));
    EXPECT_EQ((std::get<Command>(result5)), (Command{CommandType::EXPIRE, "username", std::nullopt, std::chrono::seconds(12)}));
}

// Test that a valid command works regardless of command capitalisation.
TEST(RequestParserTest, ParserParsesIgnoringCommandCapitalisation) {
    RequestParser parser;
    auto result = parser.parse("sEt username user_name");

    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ((std::get<Command>(result)), (Command{CommandType::SET, "username", "user_name", std::nullopt}));
}

// Test that a value with spaces is preserved.
TEST(RequestParserTest, ParserParsesKeyWithSpaces) {
    RequestParser parser;
    auto result = parser.parse("SET text HELLO  THERE !!!!");

    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ((std::get<Command>(result)), (Command{CommandType::SET, "text", "HELLO  THERE !!!!", std::nullopt}));
}

// Test that unicode keys and values are preserved.
TEST(RequestParserTest, ParserParsesUnicode) {
    RequestParser parser;
    auto result = parser.parse("SET 使用者名 αξία");

    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ((std::get<Command>(result)), (Command{CommandType::SET, "使用者名", "αξία", std::nullopt}));
}

// Test that long keys and long values are preserved.
TEST(RequestParserTest, ParserParsesLongKeysAndValues) {
    RequestParser parser;
    std::string start = "SET ";
    std::string space = " ";
    std::string long_key(10000, 'k');
    std::string long_value(10000, 'v');
    auto result = parser.parse(start + long_key + space + long_value);

    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ((std::get<Command>(result)), (Command{CommandType::SET, long_key, long_value, std::nullopt}));
}

// Test that a valid get command works.
TEST(RequestParserTest, ParserParsesGet) {
    RequestParser parser;
    auto result = parser.parse("GET username");

    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ((std::get<Command>(result)), (Command{CommandType::GET, "username", std::nullopt, std::nullopt}));
}

// Test that a valid delete command works.
TEST(RequestParserTest, ParserParsesDelete) {
    RequestParser parser;
    auto result = parser.parse("DELETE username");

    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ((std::get<Command>(result)), (Command{CommandType::DELETE, "username", std::nullopt, std::nullopt}));
}

// Test that a valid exists command works.
TEST(RequestParserTest, ParserParsesExists) {
    RequestParser parser;
    auto result = parser.parse("EXISTS username");

    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ((std::get<Command>(result)), (Command{CommandType::EXISTS, "username", std::nullopt, std::nullopt}));
}

// Test that a valid expires command works.
TEST(RequestParserTest, ParserParsesExpire) {
    RequestParser parser;
    auto result = parser.parse("EXPIRE username 100");

    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ((std::get<Command>(result)), (Command{CommandType::EXPIRE, "username", std::nullopt, std::chrono::seconds{100}}));
}

// Test that an EXPIRE with an invalid duration returns an error
TEST(RequestParserTest, ParsingExpireInavlidDuration) {
    RequestParser parser;
    auto result1 = parser.parse("EXPIRE username ");
    auto result2 = parser.parse("EXPIRE username 0");
    auto result3 = parser.parse("EXPIRE username -12");
    auto result4 = parser.parse("EXPIRE username 12.12");
    auto result5 = parser.parse("EXPIRE username twelve");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result1));
    ASSERT_TRUE(std::holds_alternative<ParseError>(result2));
    ASSERT_TRUE(std::holds_alternative<ParseError>(result3));
    ASSERT_TRUE(std::holds_alternative<ParseError>(result4));
    ASSERT_TRUE(std::holds_alternative<ParseError>(result5));
    EXPECT_EQ((std::get<ParseError>(result1)), ParseError::INVALID_DURATION);
    EXPECT_EQ((std::get<ParseError>(result2)), ParseError::INVALID_DURATION);
    EXPECT_EQ((std::get<ParseError>(result3)), ParseError::INVALID_DURATION);
    EXPECT_EQ((std::get<ParseError>(result4)), ParseError::INVALID_DURATION);
    EXPECT_EQ((std::get<ParseError>(result5)), ParseError::INVALID_DURATION);
}

// Test that an EXPIRE with too large a duration is rejected.
TEST(RequestParserTest, ParsingExpireDurationTooLong) {
    RequestParser parser;
    auto result = parser.parse("EXPIRE username 9999999999");

    ASSERT_TRUE(std::holds_alternative<ParseError>(result));
    EXPECT_EQ((std::get<ParseError>(result)), ParseError::INVALID_DURATION);
}

