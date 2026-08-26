#include <catch2/catch_test_macros.hpp>
#include "test/stubs/SystemStub.h"

TEST_CASE("System: init", "[System]") {
    SystemStub stub;
    System& system = stub.buildSystem();

    SECTION("connects to mqtt when a broker is configured") {
        REQUIRE(stub.settings.storeMqttBrokerConfig({"mqtt://broker.example", "user", "pass"}));

        system.initialize();

        REQUIRE(stub.mqtt.getBeginCallCount() == 1);
        REQUIRE(stub.mqtt.getLastConnectOptions().brokerUri == "mqtt://broker.example");
        REQUIRE(stub.mqtt.getLastConnectOptions().username == "user");
        REQUIRE(stub.mqtt.getLastConnectOptions().password == "pass");
        REQUIRE(stub.mqtt.getLastConnectOptions().lwtMessage == "Offline");
    }

    SECTION("does not connect to mqtt without a configured broker") {
        system.initialize();

        REQUIRE(stub.mqtt.getBeginCallCount() == 0);
    }
}

TEST_CASE("System: free", "[System]") {
    SystemStub stub;
    System& system = stub.buildSystem();

    SECTION("fails before init") {
        REQUIRE_FALSE(system.free());
    }

    SECTION("succeeds after init and releases dependencies") {
        system.initialize();

        REQUIRE(system.free());

        REQUIRE_FALSE(stub.mqtt.isReady());
    }

    SECTION("fails when called twice") {
        system.initialize();
        REQUIRE(system.free());
        REQUIRE_FALSE(system.free());
    }
}

TEST_CASE("System: onMqttConnected publishes availability", "[System]") {
    SystemStub stub;
    System& system = stub.buildSystem();

    SECTION("publishes Online on the lwt topic") {
        REQUIRE(stub.settings.storeMqttBrokerConfig({"mqtt://broker.example", "", ""}));
        REQUIRE(stub.settings.storeMqttNodeId("lasergate-01"));

        system.initialize();
        stub.mqtt.simulateConnected();

        REQUIRE(stub.mqtt.getPublishedMessages().size() == 1);
        REQUIRE(stub.mqtt.getPublishedMessages()[0].topic == "tele/lasergate/lasergate-01/LWT");
        REQUIRE(stub.mqtt.getPublishedMessages()[0].payload == "Online");
        REQUIRE(stub.mqtt.getPublishedMessages()[0].qos == 1);
        REQUIRE(stub.mqtt.getPublishedMessages()[0].retain == true);
    }

    SECTION("publishes nothing when no broker is configured") {
        system.initialize();
        stub.mqtt.simulateConnected();

        REQUIRE(stub.mqtt.getPublishedMessages().empty());
    }
}

TEST_CASE("System: update polls the mqtt activity led pulse", "[System]") {
    SystemStub stub(GPIO_NUM_3, GPIO_NUM_NC);
    stub.time.setStubbedMillis(0);
    System& system = stub.buildSystem();
    system.initialize();

    SECTION("turns the led back on 100ms after a publish, regardless of system state") {
        stub.mqtt.publish("stat/lasergate/0", "ON", 1, true);
        REQUIRE(stub.gpio.test_gpioGetLevel(GPIO_NUM_3) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        stub.time.setStubbedMillis(99);
        system.update();
        REQUIRE(stub.gpio.test_gpioGetLevel(GPIO_NUM_3) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        stub.time.setStubbedMillis(100);
        system.update();
        REQUIRE(stub.gpio.test_gpioGetLevel(GPIO_NUM_3) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("turns the led back on 100ms after a received message") {
        stub.mqtt.simulateMessage("cmnd/lasergate/x/0/POWER", "ON");
        REQUIRE(stub.gpio.test_gpioGetLevel(GPIO_NUM_3) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        stub.time.setStubbedMillis(100);
        system.update();
        REQUIRE(stub.gpio.test_gpioGetLevel(GPIO_NUM_3) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }
}
