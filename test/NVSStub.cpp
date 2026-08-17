#include <catch2/catch_test_macros.hpp>
#include "test/stubs/NVSStub.h"

TEST_CASE("NVSStub", "[NVSStub]") {
    NVSStub stub;

    SECTION("setInt fails before begin") {
        REQUIRE_FALSE(stub.setInt("count", 42));
    }

    SECTION("getInt fails before begin") {
        int32_t value = 0;
        REQUIRE_FALSE(stub.getInt("count", value));
    }

    SECTION("begin records the namespace") {
        stub.begin("lasergate");
        REQUIRE(stub.getLastNamespace() == "lasergate");
    }

    SECTION("begin increments the call count") {
        stub.begin("lasergate");
        REQUIRE(stub.getBeginCallCount() == 1);
    }

    SECTION("begin fails when already initialized") {
        REQUIRE(stub.begin("lasergate"));
        REQUIRE_FALSE(stub.begin("lasergate"));
    }

    SECTION("free fails before begin") {
        REQUIRE_FALSE(stub.free());
    }

    SECTION("free succeeds after begin") {
        stub.begin("lasergate");
        REQUIRE(stub.free());
    }

    SECTION("free fails when called twice") {
        stub.begin("lasergate");
        REQUIRE(stub.free());
        REQUIRE_FALSE(stub.free());
    }

    SECTION("setInt/getInt round trip") {
        stub.begin("lasergate");
        REQUIRE(stub.setInt("count", 42));

        int32_t value = 0;
        REQUIRE(stub.getInt("count", value));
        REQUIRE(value == 42);
    }

    SECTION("getInt fails for an unknown key") {
        stub.begin("lasergate");

        int32_t value = 0;
        REQUIRE_FALSE(stub.getInt("missing", value));
    }

    SECTION("setInt overwrites a previously stored value") {
        stub.begin("lasergate");
        stub.setInt("count", 1);
        stub.setInt("count", 2);

        int32_t value = 0;
        REQUIRE(stub.getInt("count", value));
        REQUIRE(value == 2);
    }

    SECTION("setString fails before begin") {
        REQUIRE_FALSE(stub.setString("ssid", "my_example_ap"));
    }

    SECTION("getString fails before begin") {
        char buffer[NVS_MAX_STRING_LENGTH + 1];
        REQUIRE_FALSE(stub.getString("ssid", buffer));
    }

    SECTION("setString/getString round trip") {
        stub.begin("lasergate");
        REQUIRE(stub.setString("title", "Lasergate"));

        char buffer[NVS_MAX_STRING_LENGTH + 1];
        REQUIRE(stub.getString("title", buffer));
        REQUIRE(std::string(buffer) == "Lasergate");
    }

    SECTION("getString fails for an unknown key") {
        stub.begin("lasergate");

        char buffer[NVS_MAX_STRING_LENGTH + 1];
        REQUIRE_FALSE(stub.getString("missing", buffer));
    }

    SECTION("setString fails for a value longer than NVS_MAX_STRING_LENGTH") {
        stub.begin("lasergate");
        const std::string tooLong(NVS_MAX_STRING_LENGTH + 1, 'a');
        REQUIRE_FALSE(stub.setString("title", tooLong.c_str()));
    }

    SECTION("setString accepts a value exactly NVS_MAX_STRING_LENGTH characters long") {
        stub.begin("lasergate");
        const std::string maxLength(NVS_MAX_STRING_LENGTH, 'a');
        REQUIRE(stub.setString("title", maxLength.c_str()));
    }

    SECTION("eraseKey fails before begin") {
        REQUIRE_FALSE(stub.eraseKey("title"));
    }

    SECTION("eraseKey removes a previously stored string value") {
        stub.begin("lasergate");
        stub.setString("title", "Lasergate");

        REQUIRE(stub.eraseKey("title"));

        char buffer[NVS_MAX_STRING_LENGTH + 1];
        REQUIRE_FALSE(stub.getString("title", buffer));
    }

    SECTION("eraseKey removes a previously stored int value") {
        stub.begin("lasergate");
        stub.setInt("count", 42);

        REQUIRE(stub.eraseKey("count"));

        int32_t value = 0;
        REQUIRE_FALSE(stub.getInt("count", value));
    }

    SECTION("eraseKey succeeds for a key that was never set") {
        stub.begin("lasergate");
        REQUIRE(stub.eraseKey("missing"));
    }
}
