//
// Created by Leon Etienne on 21.03.26.
//

#include <catch2/catch_test_macros.hpp>
#include "../main/include/GpioPinRegister.h"
#include "test/stubs/GpioStub.h"
#include "../main/include/platform/GpioDigitalWritePin.h"

TEST_CASE("GpioDigitalWritePin: lifecycle", "[GpioDigitalWritePin]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    GpioDigitalWritePin pin{pr, gpioStub, 19};

    SECTION("not ready by default") {
        REQUIRE_FALSE(pin.isReady());
    }

    SECTION("can initialize") {
        REQUIRE(pin.initialize());
    }

    SECTION("can't initialize twice") {
        REQUIRE(pin.initialize());
        REQUIRE_FALSE(pin.initialize());
    }

    SECTION("is ready after initialization") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.isReady());
    }

    SECTION("pin is bound after initialization") {
        REQUIRE(pin.initialize());
        REQUIRE(pr.isPinBound(19));
    }

    SECTION("pin is set to direction output after initialization") {
        REQUIRE(pin.initialize());
        REQUIRE(gpioStub.test_gpioGetMode(19) == GPIO_MODE_OUTPUT);
    }

    SECTION("initialize fails if pin is already bound") {
        REQUIRE(pr.bindPin(19));
        REQUIRE_FALSE(pin.initialize());
    }

    SECTION("pin accepts output HIGH") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.setState(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("pin reports output HIGH") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.setState(PIN_STATE_DIGITAL::HIGH));
        REQUIRE(pin.getState() == PIN_STATE_DIGITAL::HIGH);
    }

    SECTION("pin sets output to HIGH on gpio") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.setState(PIN_STATE_DIGITAL::HIGH));
        REQUIRE(gpioStub.test_gpioGetLevel(19) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("pin accepts output LOW") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.setState(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("pin reports output LOW") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.setState(PIN_STATE_DIGITAL::LOW));
        REQUIRE(pin.getState() == PIN_STATE_DIGITAL::LOW);
    }

    SECTION("pin sets output to LOW on gpio") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.setState(PIN_STATE_DIGITAL::LOW));
        REQUIRE(gpioStub.test_gpioGetLevel(19) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("can switch from HIGH to LOW") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.setState(PIN_STATE_DIGITAL::HIGH));
        REQUIRE(pin.getState() == PIN_STATE_DIGITAL::HIGH);

        REQUIRE(pin.setState(PIN_STATE_DIGITAL::LOW));
        REQUIRE(pin.getState() == PIN_STATE_DIGITAL::LOW);
        REQUIRE(gpioStub.test_gpioGetLevel(19) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("setState fails when pin is not initialized") {
        REQUIRE_FALSE(pin.setState(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("pin can be freed") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.free());
    }

    SECTION("pin is not ready after freeing") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.free());
        REQUIRE_FALSE(pin.isReady());
    }

    SECTION("pin can't be freed again") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.free());
        REQUIRE_FALSE(pin.free());
    }

    SECTION("free fails when pin is not initialized") {
        REQUIRE_FALSE(pin.free());
    }

    SECTION("pin is no longer bound after freeing") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.free());
        REQUIRE_FALSE(pr.isPinBound(19));
    }

    SECTION("free sets output to LOW before release") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.setState(PIN_STATE_DIGITAL::HIGH));
        REQUIRE(pin.free());
        REQUIRE(gpioStub.test_gpioGetLevel(19) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("moved pin transfers readiness") {
        REQUIRE(pin.initialize());

        GpioDigitalWritePin moved{std::move(pin)};

        REQUIRE(moved.isReady());
        REQUIRE_FALSE(pin.isReady());
    }

    SECTION("moved pin keeps current state") {
        REQUIRE(pin.initialize());
        REQUIRE(pin.setState(PIN_STATE_DIGITAL::HIGH));

        GpioDigitalWritePin moved{std::move(pin)};

        REQUIRE(moved.getState() == PIN_STATE_DIGITAL::HIGH);
    }
}
