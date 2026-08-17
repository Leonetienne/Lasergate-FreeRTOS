#include <catch2/catch_test_macros.hpp>
#include "test/stubs/MqttStub.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/TimeStub.h"
#include "GpioPinRegister.h"

TEST_CASE("MqttStub", "[MqttStub]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    TimeStub timeStub{};
    MqttStub stub(GPIO_NUM_NC, gpioStub, pr, timeStub);

    SECTION("default state is Disconnected") {
        REQUIRE(stub.getState() == MqttConnectionState::Disconnected);
    }

    SECTION("begin records the connect options") {
        stub.begin(MqttConnectOptions{"mqtt://broker", "user", "pass", "tele/LWT", "Offline"});
        REQUIRE(stub.getLastConnectOptions().brokerUri == "mqtt://broker");
        REQUIRE(stub.getLastConnectOptions().username == "user");
        REQUIRE(stub.getLastConnectOptions().password == "pass");
        REQUIRE(stub.getLastConnectOptions().lwtTopic == "tele/LWT");
        REQUIRE(stub.getLastConnectOptions().lwtMessage == "Offline");
    }

    SECTION("begin increments the call count") {
        stub.begin(MqttConnectOptions{});
        stub.begin(MqttConnectOptions{});
        REQUIRE(stub.getBeginCallCount() == 2);
    }

    SECTION("publish records the message") {
        stub.publish("stat/lasergate/0", "ON", 1, true);
        REQUIRE(stub.getPublishedMessages().size() == 1);
        REQUIRE(stub.getPublishedMessages()[0].topic == "stat/lasergate/0");
        REQUIRE(stub.getPublishedMessages()[0].payload == "ON");
        REQUIRE(stub.getPublishedMessages()[0].qos == 1);
        REQUIRE(stub.getPublishedMessages()[0].retain == true);
    }

    SECTION("subscribe records the topic") {
        stub.subscribe("cmnd/lasergate/0", 1);
        REQUIRE(stub.getSubscriptions().size() == 1);
        REQUIRE(stub.getSubscriptions()[0].topic == "cmnd/lasergate/0");
        REQUIRE(stub.getSubscriptions()[0].qos == 1);
    }

    SECTION("simulateConnected sets state to Connected") {
        stub.simulateConnected();
        REQUIRE(stub.getState() == MqttConnectionState::Connected);
    }

    SECTION("simulateConnected fires the onConnected callback") {
        bool called = false;
        stub.setOnConnected([&called]() { called = true; });
        stub.simulateConnected();
        REQUIRE(called);
    }

    SECTION("simulateDisconnected sets state to Disconnected") {
        stub.simulateConnected();
        stub.simulateDisconnected();
        REQUIRE(stub.getState() == MqttConnectionState::Disconnected);
    }

    SECTION("simulateDisconnected fires the onDisconnected callback") {
        bool called = false;
        stub.setOnDisconnected([&called]() { called = true; });
        stub.simulateDisconnected();
        REQUIRE(called);
    }

    SECTION("simulateMessage fires the onMessage callback with topic and payload") {
        std::string gotTopic;
        std::string gotPayload;
        stub.setOnMessage([&gotTopic, &gotPayload](const std::string& topic, const std::string& payload) {
            gotTopic = topic;
            gotPayload = payload;
        });
        stub.simulateMessage("cmnd/lasergate/0", "ON");
        REQUIRE(gotTopic == "cmnd/lasergate/0");
        REQUIRE(gotPayload == "ON");
    }

    SECTION("free resets state to Disconnected") {
        stub.simulateConnected();
        stub.free();
        REQUIRE(stub.getState() == MqttConnectionState::Disconnected);
    }
}

TEST_CASE("MqttStub: activity indicator LED", "[MqttStub]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    TimeStub timeStub{};

    SECTION("a configured pin is bound and defaults to HIGH") {
        MqttStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        REQUIRE(pr.isPinBound(GPIO_NUM_2));
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("GPIO_NUM_NC leaves no pin bound") {
        MqttStub stub(GPIO_NUM_NC, gpioStub, pr, timeStub);
        REQUIRE_FALSE(pr.isPinBound(GPIO_NUM_2));
    }

    SECTION("publish pulses the led LOW immediately") {
        MqttStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.publish("stat/lasergate/0", "ON", 1, true);
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("simulateMessage pulses the led LOW immediately") {
        MqttStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.simulateMessage("cmnd/lasergate/0", "ON");
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("updateActivityLedPulse leaves the led off before 100ms elapse") {
        timeStub.setStubbedMillis(0);
        MqttStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.publish("stat/lasergate/0", "ON", 1, true);

        timeStub.setStubbedMillis(99);
        stub.updateActivityLedPulse();

        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("updateActivityLedPulse turns the led back on once 100ms have elapsed") {
        timeStub.setStubbedMillis(0);
        MqttStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.publish("stat/lasergate/0", "ON", 1, true);

        timeStub.setStubbedMillis(100);
        stub.updateActivityLedPulse();

        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("a new message re-triggers the pulse after it already turned back on") {
        timeStub.setStubbedMillis(0);
        MqttStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.publish("stat/lasergate/0", "ON", 1, true);

        timeStub.setStubbedMillis(100);
        stub.updateActivityLedPulse();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));

        stub.simulateMessage("cmnd/lasergate/0", "ON");
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        timeStub.setStubbedMillis(199);
        stub.updateActivityLedPulse();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        timeStub.setStubbedMillis(200);
        stub.updateActivityLedPulse();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("updateActivityLedPulse does nothing without a configured pin") {
        MqttStub stub(GPIO_NUM_NC, gpioStub, pr, timeStub);
        timeStub.setStubbedMillis(10000);
        stub.updateActivityLedPulse();
        SUCCEED("no crash, nothing to assert without a pin");
    }

    SECTION("simulateConnected turns the led on") {
        MqttStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.free();
        stub.simulateConnected();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("simulateDisconnected turns the led off") {
        MqttStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.simulateConnected();
        stub.simulateDisconnected();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("free turns the led off") {
        MqttStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.simulateConnected();
        stub.free();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("a disconnect during an active pulse cancels the pending revert to on") {
        timeStub.setStubbedMillis(0);
        MqttStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.simulateConnected();
        stub.simulateMessage("cmnd/lasergate/0", "ON");
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        stub.simulateDisconnected();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        timeStub.setStubbedMillis(100);
        stub.updateActivityLedPulse();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }
}
