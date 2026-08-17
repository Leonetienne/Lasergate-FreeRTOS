#include <catch2/catch_test_macros.hpp>
#include "test/stubs/TimeStub.h"

TEST_CASE("TimeStub", "[TimeStub]") {
    TimeStub timeStub;

    SECTION("default time is the stubbed initial value") {
        REQUIRE(timeStub.getTime() == 1700000000);
    }

    SECTION("setStubbedTime changes the time returned by getTime") {
        timeStub.setStubbedTime(42);
        REQUIRE(timeStub.getTime() == 42);
    }

    SECTION("getSecondsSince returns the correct elapsed seconds") {
        REQUIRE(timeStub.getSecondsSince(1699999900) == 100);
    }

    SECTION("setStubbedMillis changes the millis returned by getMillis") {
        timeStub.setStubbedMillis(1337);
        REQUIRE(timeStub.getMillis() == 1337);
    }
}
