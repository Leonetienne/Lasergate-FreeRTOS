#include <catch2/catch_test_macros.hpp>
#include "UrlEncodedForm.h"

TEST_CASE("UrlEncodedForm: parse", "[UrlEncodedForm]") {
    SECTION("parses multiple pairs") {
        const auto values = UrlEncodedForm::parse("a=1&b=2&c=3");
        REQUIRE(values.at("a") == "1");
        REQUIRE(values.at("b") == "2");
        REQUIRE(values.at("c") == "3");
    }

    SECTION("decodes percent-encoded characters in keys and values") {
        const auto values = UrlEncodedForm::parse("na%20me=Jo%40hn");
        REQUIRE(values.at("na me") == "Jo@hn");
    }

    SECTION("decodes plus signs as spaces") {
        const auto values = UrlEncodedForm::parse("a=hello+world");
        REQUIRE(values.at("a") == "hello world");
    }

    SECTION("allows an empty value") {
        const auto values = UrlEncodedForm::parse("a=");
        REQUIRE(values.at("a").empty());
    }

    SECTION("last occurrence of a duplicate key wins") {
        const auto values = UrlEncodedForm::parse("a=1&a=2");
        REQUIRE(values.at("a") == "2");
    }

    SECTION("ignores a pair without an equals sign") {
        const auto values = UrlEncodedForm::parse("a=1&justakey&b=2");
        REQUIRE(values.size() == 2);
        REQUIRE(values.at("a") == "1");
        REQUIRE(values.at("b") == "2");
    }

    SECTION("empty body yields no pairs") {
        REQUIRE(UrlEncodedForm::parse("").empty());
    }
}
