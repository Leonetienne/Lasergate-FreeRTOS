#include <catch2/catch_test_macros.hpp>
#include "test/stubs/NVSStub.h"
#include "../main/include/SettingsManager.h"

TEST_CASE("SettingsManager: title", "[SettingsManager]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);

    SECTION("round trip stores and retrieves the title") {
        REQUIRE(settings.storeTitle("Lasergate"));

        const auto result = settings.retrieveTitle();
        REQUIRE(result.has_value());
        REQUIRE(*result == "Lasergate");
    }

    SECTION("retrieve fails when nothing was stored") {
        REQUIRE_FALSE(settings.retrieveTitle().has_value());
    }
}

TEST_CASE("SettingsManager: mqtt broker config", "[SettingsManager]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);

    SECTION("round trip stores and retrieves the broker config") {
        REQUIRE(settings.storeMqttBrokerConfig({"mqtt://broker.example", "user", "pass"}));

        const auto result = settings.retrieveMqttBrokerConfig();
        REQUIRE(result.has_value());
        REQUIRE(result->uri == "mqtt://broker.example");
        REQUIRE(result->username == "user");
        REQUIRE(result->password == "pass");
    }

    SECTION("retrieve fails when nothing was stored") {
        REQUIRE_FALSE(settings.retrieveMqttBrokerConfig().has_value());
    }
}

TEST_CASE("SettingsManager: mqtt node id", "[SettingsManager]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);

    SECTION("round trip stores and retrieves the node id") {
        REQUIRE(settings.storeMqttNodeId("lasergate-01"));

        const auto result = settings.retrieveMqttNodeId();
        REQUIRE(result.has_value());
        REQUIRE(*result == "lasergate-01");
    }

    SECTION("retrieve fails when nothing was stored") {
        REQUIRE_FALSE(settings.retrieveMqttNodeId().has_value());
    }
}

TEST_CASE("SettingsManager: mqtt led gpio pin", "[SettingsManager]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);

    SECTION("round trip stores and retrieves the pin") {
        REQUIRE(settings.storeMqttLedGpioPin(GPIO_NUM_3));

        const auto result = settings.retrieveMqttLedGpioPin();
        REQUIRE(result.has_value());
        REQUIRE(*result == GPIO_NUM_3);
    }

    SECTION("retrieve fails when nothing was stored") {
        REQUIRE_FALSE(settings.retrieveMqttLedGpioPin().has_value());
    }
}
