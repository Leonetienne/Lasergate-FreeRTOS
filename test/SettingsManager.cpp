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

TEST_CASE("SettingsManager: ethernet led gpio pin", "[SettingsManager]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);

    SECTION("round trip stores and retrieves the pin") {
        REQUIRE(settings.storeEthernetLedGpioPin(GPIO_NUM_4));

        const auto result = settings.retrieveEthernetLedGpioPin();
        REQUIRE(result.has_value());
        REQUIRE(*result == GPIO_NUM_4);
    }

    SECTION("retrieve fails when nothing was stored") {
        REQUIRE_FALSE(settings.retrieveEthernetLedGpioPin().has_value());
    }
}

TEST_CASE("SettingsManager: gate module laser gpio pin", "[SettingsManager]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);

    SECTION("round trip stores and retrieves the pin") {
        REQUIRE(settings.storeGateModuleLaserGpioPin(0, GPIO_NUM_5));

        const auto result = settings.retrieveGateModuleLaserGpioPin(0);
        REQUIRE(result.has_value());
        REQUIRE(*result == GPIO_NUM_5);
    }

    SECTION("retrieve fails when nothing was stored") {
        REQUIRE_FALSE(settings.retrieveGateModuleLaserGpioPin(0).has_value());
    }

    SECTION("different module indices are stored independently") {
        REQUIRE(settings.storeGateModuleLaserGpioPin(0, GPIO_NUM_5));
        REQUIRE(settings.storeGateModuleLaserGpioPin(1, GPIO_NUM_6));

        REQUIRE(*settings.retrieveGateModuleLaserGpioPin(0) == GPIO_NUM_5);
        REQUIRE(*settings.retrieveGateModuleLaserGpioPin(1) == GPIO_NUM_6);
    }
}

TEST_CASE("SettingsManager: gate module led gpio pin", "[SettingsManager]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);

    SECTION("round trip stores and retrieves the pin") {
        REQUIRE(settings.storeGateModuleLedGpioPin(0, GPIO_NUM_7));

        const auto result = settings.retrieveGateModuleLedGpioPin(0);
        REQUIRE(result.has_value());
        REQUIRE(*result == GPIO_NUM_7);
    }

    SECTION("retrieve fails when nothing was stored") {
        REQUIRE_FALSE(settings.retrieveGateModuleLedGpioPin(0).has_value());
    }

    SECTION("different module indices are stored independently") {
        REQUIRE(settings.storeGateModuleLedGpioPin(0, GPIO_NUM_7));
        REQUIRE(settings.storeGateModuleLedGpioPin(1, GPIO_NUM_8));

        REQUIRE(*settings.retrieveGateModuleLedGpioPin(0) == GPIO_NUM_7);
        REQUIRE(*settings.retrieveGateModuleLedGpioPin(1) == GPIO_NUM_8);
    }
}

TEST_CASE("SettingsManager: gate module ldr gpio pin", "[SettingsManager]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);

    SECTION("round trip stores and retrieves the pin") {
        REQUIRE(settings.storeGateModuleLdrGpioPin(0, GPIO_NUM_34));

        const auto result = settings.retrieveGateModuleLdrGpioPin(0);
        REQUIRE(result.has_value());
        REQUIRE(*result == GPIO_NUM_34);
    }

    SECTION("retrieve fails when nothing was stored") {
        REQUIRE_FALSE(settings.retrieveGateModuleLdrGpioPin(0).has_value());
    }

    SECTION("different module indices are stored independently") {
        REQUIRE(settings.storeGateModuleLdrGpioPin(0, GPIO_NUM_34));
        REQUIRE(settings.storeGateModuleLdrGpioPin(1, GPIO_NUM_35));

        REQUIRE(*settings.retrieveGateModuleLdrGpioPin(0) == GPIO_NUM_34);
        REQUIRE(*settings.retrieveGateModuleLdrGpioPin(1) == GPIO_NUM_35);
    }
}

TEST_CASE("SettingsManager: connectivity leds enabled", "[SettingsManager]") {
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);

    SECTION("round trip stores and retrieves true") {
        REQUIRE(settings.storeConnLedsEnabled(true));

        const auto result = settings.retrieveConnLedsEnabled();
        REQUIRE(result.has_value());
        REQUIRE(*result == true);
    }

    SECTION("round trip stores and retrieves false") {
        REQUIRE(settings.storeConnLedsEnabled(false));

        const auto result = settings.retrieveConnLedsEnabled();
        REQUIRE(result.has_value());
        REQUIRE(*result == false);
    }

    SECTION("retrieve fails when nothing was stored") {
        REQUIRE_FALSE(settings.retrieveConnLedsEnabled().has_value());
    }
}
