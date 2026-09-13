#include <catch2/catch_test_macros.hpp>
#include "JsonWriter.h"

TEST_CASE("JsonWriter: escape", "[JsonWriter]") {
    SECTION("passes plain text through unchanged") {
        REQUIRE(JsonWriter::escape("Lasergate") == "Lasergate");
    }

    SECTION("escapes quotes and backslashes") {
        REQUIRE(JsonWriter::escape("say \"hi\"") == "say \\\"hi\\\"");
        REQUIRE(JsonWriter::escape("a\\b") == "a\\\\b");
    }

    SECTION("escapes newlines, carriage returns and tabs") {
        REQUIRE(JsonWriter::escape("a\nb\rc\td") == "a\\nb\\rc\\td");
    }

    SECTION("escapes other control characters as \\u00XX") {
        REQUIRE(JsonWriter::escape(std::string(1, '\x01')) == "\\u0001");
    }

    SECTION("empty string escapes to empty string") {
        REQUIRE(JsonWriter::escape("").empty());
    }
}

TEST_CASE("JsonWriter: string", "[JsonWriter]") {
    SECTION("wraps escaped text in quotes") {
        REQUIRE(JsonWriter::string("hello") == "\"hello\"");
        REQUIRE(JsonWriter::string("a\"b") == "\"a\\\"b\"");
    }
}

TEST_CASE("JsonWriter: boolean", "[JsonWriter]") {
    REQUIRE(std::string(JsonWriter::boolean(true)) == "true");
    REQUIRE(std::string(JsonWriter::boolean(false)) == "false");
}

TEST_CASE("JsonWriter: number", "[JsonWriter]") {
    REQUIRE(JsonWriter::number(0) == "0");
    REQUIRE(JsonWriter::number(42) == "42");
    REQUIRE(JsonWriter::number(-1) == "-1");
}

TEST_CASE("JsonWriter: null", "[JsonWriter]") {
    REQUIRE(std::string(JsonWriter::null()) == "null");
}

TEST_CASE("JsonWriter: optNumber", "[JsonWriter]") {
    SECTION("formats a present value") {
        REQUIRE(JsonWriter::optNumber(std::optional<uint16_t>(7)) == "7");
    }

    SECTION("formats an empty optional as null") {
        REQUIRE(JsonWriter::optNumber(std::optional<uint16_t>()) == "null");
    }
}

TEST_CASE("JsonWriter: optBoolean", "[JsonWriter]") {
    SECTION("formats a present value") {
        REQUIRE(JsonWriter::optBoolean(std::optional<bool>(true)) == "true");
        REQUIRE(JsonWriter::optBoolean(std::optional<bool>(false)) == "false");
    }

    SECTION("formats an empty optional as null") {
        REQUIRE(JsonWriter::optBoolean(std::optional<bool>()) == "null");
    }
}
