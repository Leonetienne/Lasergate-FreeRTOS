#include <catch2/catch_test_macros.hpp>
#include "test/stubs/EthernetManagerStub.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/TimeStub.h"
#include "GpioPinRegister.h"

TEST_CASE("EthernetManagerStub", "[EthernetManagerStub]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    TimeStub timeStub{};
    EthernetManagerStub stub(GPIO_NUM_NC, gpioStub, pr, timeStub);

    SECTION("default state is Disconnected") {
        REQUIRE(stub.getState() == EthernetConnectionState::Disconnected);
    }

    SECTION("begin increments the call count") {
        stub.begin();
        stub.begin();
        REQUIRE(stub.getBeginCallCount() == 2);
    }

    SECTION("begin moves state to Connecting") {
        stub.begin();
        REQUIRE(stub.getState() == EthernetConnectionState::Connecting);
    }

    SECTION("simulateConnected sets state to Connected") {
        stub.simulateConnected();
        REQUIRE(stub.getState() == EthernetConnectionState::Connected);
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
        REQUIRE(stub.getState() == EthernetConnectionState::Disconnected);
    }

    SECTION("simulateDisconnected fires the onDisconnected callback") {
        bool called = false;
        stub.setOnDisconnected([&called]() { called = true; });
        stub.simulateDisconnected();
        REQUIRE(called);
    }

    SECTION("free resets state to Disconnected") {
        stub.begin();
        stub.simulateConnected();
        stub.free();
        REQUIRE(stub.getState() == EthernetConnectionState::Disconnected);
    }
}

TEST_CASE("EthernetManagerStub: connectivity indicator LED", "[EthernetManagerStub]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    TimeStub timeStub{};

    SECTION("a configured pin is bound and defaults to LOW") {
        EthernetManagerStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        REQUIRE(pr.isPinBound(GPIO_NUM_2));
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("GPIO_NUM_NC leaves no pin bound") {
        EthernetManagerStub stub(GPIO_NUM_NC, gpioStub, pr, timeStub);
        REQUIRE_FALSE(pr.isPinBound(GPIO_NUM_2));
    }

    SECTION("goes HIGH on simulateConnected") {
        EthernetManagerStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.simulateConnected();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("goes LOW on simulateDisconnected after having been connected") {
        EthernetManagerStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.simulateConnected();
        stub.simulateDisconnected();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("goes LOW on free after having been connected") {
        EthernetManagerStub stub(GPIO_NUM_2, gpioStub, pr, timeStub);
        stub.simulateConnected();
        stub.free();
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_2) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }
}
