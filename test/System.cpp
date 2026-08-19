#include <catch2/catch_test_macros.hpp>
#include "System.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "SettingsManager.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/TimeStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/MqttStub.h"
#include "test/stubs/EthernetManagerStub.h"
#include "test/stubs/HttpServerStub.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/RandomStub.h"

TEST_CASE("System: init", "[System]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    StateMachine stateMachine{};
    SettingsManager settings(nvs);
    MqttStub mqttStub(GPIO_NUM_NC, gpioStub, pr, timeStub);
    EthernetManagerStub ethernetStub(GPIO_NUM_NC, gpioStub, pr, timeStub);
    HttpServerStub httpServerStub;

    System system(stateMachine, pr, gpioStub, adcStub, randomStub, timeStub, nvs, settings, mqttStub, ethernetStub, httpServerStub);

    SECTION("connects to mqtt when a broker is configured") {
        REQUIRE(settings.storeMqttBrokerConfig({"mqtt://broker.example", "user", "pass"}));

        system.initialize();

        REQUIRE(mqttStub.getBeginCallCount() == 1);
        REQUIRE(mqttStub.getLastConnectOptions().brokerUri == "mqtt://broker.example");
        REQUIRE(mqttStub.getLastConnectOptions().username == "user");
        REQUIRE(mqttStub.getLastConnectOptions().password == "pass");
        REQUIRE(mqttStub.getLastConnectOptions().lwtMessage == "Offline");
    }

    SECTION("does not connect to mqtt without a configured broker") {
        system.initialize();

        REQUIRE(mqttStub.getBeginCallCount() == 0);
    }
}

TEST_CASE("System: free", "[System]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    StateMachine stateMachine{};
    SettingsManager settings(nvs);
    MqttStub mqttStub(GPIO_NUM_NC, gpioStub, pr, timeStub);
    EthernetManagerStub ethernetStub(GPIO_NUM_NC, gpioStub, pr, timeStub);
    HttpServerStub httpServerStub;

    System system(stateMachine, pr, gpioStub, adcStub, randomStub, timeStub, nvs, settings, mqttStub, ethernetStub, httpServerStub);

    SECTION("fails before init") {
        REQUIRE_FALSE(system.free());
    }

    SECTION("succeeds after init and releases dependencies") {
        system.initialize();

        REQUIRE(system.free());

        REQUIRE_FALSE(mqttStub.isReady());
    }

    SECTION("fails when called twice") {
        system.initialize();
        REQUIRE(system.free());
        REQUIRE_FALSE(system.free());
    }
}

TEST_CASE("System: onMqttConnected publishes availability", "[System]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    StateMachine stateMachine{};
    SettingsManager settings(nvs);
    MqttStub mqttStub(GPIO_NUM_NC, gpioStub, pr, timeStub);
    EthernetManagerStub ethernetStub(GPIO_NUM_NC, gpioStub, pr, timeStub);
    HttpServerStub httpServerStub;

    System system(stateMachine, pr, gpioStub, adcStub, randomStub, timeStub, nvs, settings, mqttStub, ethernetStub, httpServerStub);

    SECTION("publishes Online on the lwt topic") {
        REQUIRE(settings.storeMqttBrokerConfig({"mqtt://broker.example", "", ""}));
        REQUIRE(settings.storeMqttNodeId("lasergate-01"));

        system.initialize();
        mqttStub.simulateConnected();

        REQUIRE(mqttStub.getPublishedMessages().size() == 1);
        REQUIRE(mqttStub.getPublishedMessages()[0].topic == "tele/lasergate/lasergate-01/LWT");
        REQUIRE(mqttStub.getPublishedMessages()[0].payload == "Online");
        REQUIRE(mqttStub.getPublishedMessages()[0].qos == 1);
        REQUIRE(mqttStub.getPublishedMessages()[0].retain == true);
    }

    SECTION("publishes nothing when no broker is configured") {
        system.initialize();
        mqttStub.simulateConnected();

        REQUIRE(mqttStub.getPublishedMessages().empty());
    }
}

TEST_CASE("System: update polls the mqtt activity led pulse", "[System]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    timeStub.setStubbedMillis(0);
    StateMachine stateMachine{};
    SettingsManager settings(nvs);
    MqttStub mqttStub(GPIO_NUM_3, gpioStub, pr, timeStub);
    EthernetManagerStub ethernetStub(GPIO_NUM_NC, gpioStub, pr, timeStub);
    HttpServerStub httpServerStub;

    System system(stateMachine, pr, gpioStub, adcStub, randomStub, timeStub, nvs, settings, mqttStub, ethernetStub, httpServerStub);
    system.initialize();

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
