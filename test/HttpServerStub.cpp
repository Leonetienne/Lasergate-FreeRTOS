#include <catch2/catch_test_macros.hpp>
#include "test/stubs/HttpServerStub.h"

TEST_CASE("HttpServerStub", "[HttpServerStub]") {
    HttpServerStub stub{};

    SECTION("default state is not running") {
        REQUIRE_FALSE(stub.test_isRunning());
    }

    SECTION("begin starts it and counts the call") {
        REQUIRE(stub.begin());
        REQUIRE(stub.test_isRunning());
        REQUIRE(stub.test_getBeginCallCount() == 1);
    }

    SECTION("begin fails when already running") {
        REQUIRE(stub.begin());
        REQUIRE_FALSE(stub.begin());
        REQUIRE(stub.test_getBeginCallCount() == 2);
    }

    SECTION("free stops it and counts the call") {
        REQUIRE(stub.begin());
        REQUIRE(stub.free());
        REQUIRE_FALSE(stub.test_isRunning());
        REQUIRE(stub.test_getFreeCallCount() == 1);
    }

    SECTION("free fails when not running") {
        REQUIRE_FALSE(stub.free());
        REQUIRE(stub.test_getFreeCallCount() == 1);
    }
}
