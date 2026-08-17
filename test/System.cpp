#include <catch2/catch_test_macros.hpp>
#include "System.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "SettingsManager.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/TimeStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/MqttStub.h"

TEST_CASE("System: init", "[System]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    StateMachine stateMachine{};
    SettingsManager settings(nvs);
    MqttStub mqttStub(GPIO_NUM_NC, gpioStub, pr, timeStub);

    System system(stateMachine, pr, gpioStub, timeStub, nvs, settings, mqttStub);

    SECTION("initializes the laser test pin as output") {
        system.init();

        REQUIRE(pr.isPinBound(GPIO_NUM_39));
        REQUIRE(gpioStub.test_gpioGetMode(GPIO_NUM_39) == GPIO_MODE_OUTPUT);
    }

    SECTION("connects to mqtt when a broker is configured") {
        REQUIRE(settings.storeMqttBrokerConfig({"mqtt://broker.example", "user", "pass"}));

        system.init();

        REQUIRE(mqttStub.getBeginCallCount() == 1);
        REQUIRE(mqttStub.getLastConnectOptions().brokerUri == "mqtt://broker.example");
        REQUIRE(mqttStub.getLastConnectOptions().username == "user");
        REQUIRE(mqttStub.getLastConnectOptions().password == "pass");
        REQUIRE(mqttStub.getLastConnectOptions().lwtMessage == "Offline");
    }

    SECTION("does not connect to mqtt without a configured broker") {
        system.init();

        REQUIRE(mqttStub.getBeginCallCount() == 0);
    }
}

TEST_CASE("System: free", "[System]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    StateMachine stateMachine{};
    SettingsManager settings(nvs);
    MqttStub mqttStub(GPIO_NUM_NC, gpioStub, pr, timeStub);

    System system(stateMachine, pr, gpioStub, timeStub, nvs, settings, mqttStub);

    SECTION("fails before init") {
        REQUIRE_FALSE(system.free());
    }

    SECTION("succeeds after init and releases dependencies") {
        system.init();

        REQUIRE(system.free());

        REQUIRE_FALSE(pr.isPinBound(GPIO_NUM_39));
        REQUIRE_FALSE(mqttStub.isReady());
    }

    SECTION("fails when called twice") {
        system.init();
        REQUIRE(system.free());
        REQUIRE_FALSE(system.free());
    }
}

TEST_CASE("System: update toggles the laser test pin once per second", "[System]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    timeStub.setStubbedMillis(0);
    StateMachine stateMachine{};
    SettingsManager settings(nvs);
    MqttStub mqttStub(GPIO_NUM_NC, gpioStub, pr, timeStub);

    System system(stateMachine, pr, gpioStub, timeStub, nvs, settings, mqttStub);
    system.init();

    SECTION("starts LOW") {
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_39) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("does not toggle before one second has elapsed") {
        timeStub.setStubbedMillis(999);
        system.update();

        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_39) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("toggles HIGH after one second") {
        timeStub.setStubbedMillis(1000);
        system.update();

        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_39) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("toggles back LOW after another second") {
        timeStub.setStubbedMillis(1000);
        system.update();
        timeStub.setStubbedMillis(2000);
        system.update();

        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_39) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }
}

TEST_CASE("System: onMqttConnected publishes availability", "[System]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    StateMachine stateMachine{};
    SettingsManager settings(nvs);
    MqttStub mqttStub(GPIO_NUM_NC, gpioStub, pr, timeStub);

    System system(stateMachine, pr, gpioStub, timeStub, nvs, settings, mqttStub);

    SECTION("publishes Online on the lwt topic") {
        REQUIRE(settings.storeMqttBrokerConfig({"mqtt://broker.example", "", ""}));
        REQUIRE(settings.storeMqttNodeId("lasergate-01"));

        system.init();
        mqttStub.simulateConnected();

        REQUIRE(mqttStub.getPublishedMessages().size() == 1);
        REQUIRE(mqttStub.getPublishedMessages()[0].topic == "tele/lasergate/lasergate-01/LWT");
        REQUIRE(mqttStub.getPublishedMessages()[0].payload == "Online");
        REQUIRE(mqttStub.getPublishedMessages()[0].qos == 1);
        REQUIRE(mqttStub.getPublishedMessages()[0].retain == true);
    }

    SECTION("publishes nothing when no broker is configured") {
        system.init();
        mqttStub.simulateConnected();

        REQUIRE(mqttStub.getPublishedMessages().empty());
    }
}

TEST_CASE("System: update polls the mqtt activity led pulse", "[System]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    timeStub.setStubbedMillis(0);
    StateMachine stateMachine{};
    SettingsManager settings(nvs);
    MqttStub mqttStub(GPIO_NUM_3, gpioStub, pr, timeStub);

    System system(stateMachine, pr, gpioStub, timeStub, nvs, settings, mqttStub);
    system.init();

    SECTION("turns the led back on 100ms after a publish, regardless of system state") {
        mqttStub.publish("stat/lasergate/0", "ON", 1, true);
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_3) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        timeStub.setStubbedMillis(99);
        system.update();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_3) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        timeStub.setStubbedMillis(100);
        system.update();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_3) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("turns the led back on 100ms after a received message") {
        mqttStub.simulateMessage("cmnd/lasergate/x/0/POWER", "ON");
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_3) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        timeStub.setStubbedMillis(100);
        system.update();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_3) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }
}
