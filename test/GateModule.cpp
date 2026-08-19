#include <catch2/catch_test_macros.hpp>
#include "GateModule.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/TimeStub.h"

TEST_CASE("GateModule: lifecycle", "[GateModule]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_34;

    GateModule module(stateMachine, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);

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

TEST_CASE("GateModule: isConfigured", "[GateModule]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};

    SECTION("true when the laser and ldr pins are both real, regardless of the status led") {
        GateModule module(stateMachine, pr, gpioStub, adcStub, randomStub, timeStub, GPIO_NUM_16, GPIO_NUM_NC, GPIO_NUM_34);
        REQUIRE(module.isConfigured());
    }

    SECTION("false when the laser pin is GPIO_NUM_NC") {
        GateModule module(stateMachine, pr, gpioStub, adcStub, randomStub, timeStub, GPIO_NUM_NC, GPIO_NUM_17, GPIO_NUM_34);
        REQUIRE_FALSE(module.isConfigured());
    }

    SECTION("false when the ldr pin is GPIO_NUM_NC") {
        GateModule module(stateMachine, pr, gpioStub, adcStub, randomStub, timeStub, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_NC);
        REQUIRE_FALSE(module.isConfigured());
    }
}

TEST_CASE("GateModule: status led is optional", "[GateModule]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ldrPin = GPIO_NUM_34;

    GateModule module(stateMachine, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, GPIO_NUM_NC, ldrPin);

    SECTION("initializes successfully without a configured status led") {
        REQUIRE(module.initialize());
        REQUIRE(module.isReady());
    }

    SECTION("does not bind a pin for the unconfigured status led") {
        REQUIRE(module.initialize());
        REQUIRE(pr.isPinBound(laserPin));
        REQUIRE_FALSE(pr.isPinBound(GPIO_NUM_NC));
    }

    SECTION("freeing does not fail on account of the never-initialized status led") {
        REQUIRE(module.initialize());
        REQUIRE(module.free());
    }
}

TEST_CASE("GateModule: state dispatch", "[GateModule]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_34;

    GateModule module(stateMachine, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);
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
