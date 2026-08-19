#include <catch2/catch_test_macros.hpp>
#include "GateModule.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/AdcOneshotStub.h"

TEST_CASE("GateModule: lifecycle", "[GateModule]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    StateMachine stateMachine{};

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_34;

    GateModule module(stateMachine, pr, gpioStub, adcStub, laserPin, ledPin, ldrPin);

    SECTION("not ready by default") {
        REQUIRE_FALSE(module.isReady());
    }

    SECTION("can initialize") {
        REQUIRE(module.initialize());
        REQUIRE(module.isReady());
    }

    SECTION("can't initialize twice") {
        REQUIRE(module.initialize());
        REQUIRE_FALSE(module.initialize());
    }

    SECTION("can be freed after initializing") {
        REQUIRE(module.initialize());
        REQUIRE(module.free());
        REQUIRE_FALSE(module.isReady());
    }

    SECTION("free fails when not initialized") {
        REQUIRE_FALSE(module.free());
    }
}

TEST_CASE("GateModule: state dispatch", "[GateModule]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    StateMachine stateMachine{};

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_34;

    GateModule module(stateMachine, pr, gpioStub, adcStub, laserPin, ledPin, ldrPin);
    REQUIRE(module.initialize());

    SECTION("USER_ADJUSTING_BEAMS turns the laser on and the status led off") {
        stateMachine.setState(STATE::DISARMED);
        module.onStateChange();
        stateMachine.setState(STATE::USER_ADJUSTING_BEAMS);
        module.onStateChange();

        REQUIRE(gpioStub.test_gpioGetLevel(laserPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
        REQUIRE(gpioStub.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("FAULT turns the laser back off") {
        stateMachine.setState(STATE::DISARMED);
        module.onStateChange();
        stateMachine.setState(STATE::USER_ADJUSTING_BEAMS);
        module.onStateChange();
        REQUIRE(gpioStub.test_gpioGetLevel(laserPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));

        stateMachine.setState(STATE::FAULT);
        module.onStateChange();
        REQUIRE(gpioStub.test_gpioGetLevel(laserPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }
}
