#include <catch2/catch_test_macros.hpp>
#include "../main/include/LightEmittingDiode.h"
#include "../main/include/GpioPinRegister.h"
#include "test/stubs/GpioStub.h"

TEST_CASE("LightEmittingDiode: lifecycle", "[LightEmittingDiode]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    constexpr gpio_num_t bindPin = GPIO_NUM_19;
    LightEmittingDiode diode{pr, gpioStub, bindPin};

    SECTION("not ready by default") {
        REQUIRE_FALSE(diode.isReady());
    }

    SECTION("can initialize") {
        REQUIRE(diode.initialize());
    }

    SECTION("can't initialize twice") {
        REQUIRE(diode.initialize());
        REQUIRE_FALSE(diode.initialize());
    }

    SECTION("is ready after initialization") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.isReady());
    }

    SECTION("initialize fails if underlying gpio pin fails to initialize") {
        REQUIRE(pr.bindPin(bindPin));
        REQUIRE_FALSE(diode.initialize());
    }

    SECTION("turnOn fails when not initialized") {
        REQUIRE_FALSE(diode.turnOn());
    }

    SECTION("turnOff fails when not initialized") {
        REQUIRE_FALSE(diode.turnOff());
    }

    SECTION("setPowerState fails when not initialized") {
        REQUIRE_FALSE(diode.setPowerState(true));
    }

    SECTION("getPowerState is unexpected when not initialized") {
        auto state = diode.getPowerState();
        REQUIRE_FALSE(state.has_value());
        REQUIRE_FALSE(state.error());
    }

    SECTION("turnOn sets the gpio pin HIGH") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.turnOn());
        REQUIRE(gpioStub.test_gpioGetLevel(bindPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("turnOff sets the gpio pin LOW") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.turnOn());
        REQUIRE(diode.turnOff());
        REQUIRE(gpioStub.test_gpioGetLevel(bindPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("setPowerState(true) turns the gpio pin on") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.setPowerState(true));
        REQUIRE(gpioStub.test_gpioGetLevel(bindPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("setPowerState(false) turns the gpio pin off") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.setPowerState(true));
        REQUIRE(diode.setPowerState(false));
        REQUIRE(gpioStub.test_gpioGetLevel(bindPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("diode can be freed") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.free());
    }

    SECTION("diode is not ready after freeing") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.free());
        REQUIRE_FALSE(diode.isReady());
    }

    SECTION("diode can't be freed again") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.free());
        REQUIRE_FALSE(diode.free());
    }

    SECTION("free fails when not initialized") {
        REQUIRE_FALSE(diode.free());
    }
}
