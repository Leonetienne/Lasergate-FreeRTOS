#include <catch2/catch_test_macros.hpp>
#include "Gate.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "SettingsManager.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/TimeStub.h"

TEST_CASE("Gate: lifecycle", "[Gate]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    StateMachine stateMachine{};

    REQUIRE(settings.storeGateModuleLaserGpioPin(0, GPIO_NUM_1));
    REQUIRE(settings.storeGateModuleLedGpioPin(0, GPIO_NUM_2));
    REQUIRE(settings.storeGateModuleLdrGpioPin(0, GPIO_NUM_34));
    REQUIRE(settings.storeGateModuleLaserGpioPin(1, GPIO_NUM_3));
    REQUIRE(settings.storeGateModuleLedGpioPin(1, GPIO_NUM_4));
    REQUIRE(settings.storeGateModuleLdrGpioPin(1, GPIO_NUM_35));
    REQUIRE(settings.storeGateModuleLaserGpioPin(2, GPIO_NUM_5));
    REQUIRE(settings.storeGateModuleLedGpioPin(2, GPIO_NUM_6));
    REQUIRE(settings.storeGateModuleLdrGpioPin(2, GPIO_NUM_36));
    REQUIRE(settings.storeGateModuleLaserGpioPin(3, GPIO_NUM_7));
    REQUIRE(settings.storeGateModuleLedGpioPin(3, GPIO_NUM_8));
    REQUIRE(settings.storeGateModuleLdrGpioPin(3, GPIO_NUM_37));

    Gate gate(stateMachine, settings, pr, gpioStub, adcStub, randomStub, timeStub);

    SECTION("not ready by default") {
        REQUIRE_FALSE(gate.isReady());
    }

    SECTION("initializes all modules from configured pins") {
        REQUIRE(gate.initialize());
        REQUIRE(gate.isReady());
    }

    SECTION("can't initialize twice") {
        REQUIRE(gate.initialize());
        REQUIRE_FALSE(gate.initialize());
    }

    SECTION("can be freed after initializing") {
        REQUIRE(gate.initialize());
        REQUIRE(gate.free());
        REQUIRE_FALSE(gate.isReady());
    }

    SECTION("free fails when not initialized") {
        REQUIRE_FALSE(gate.free());
    }

    SECTION("fixedUpdate and onStateChange don't crash before initialization") {
        gate.fixedUpdate();
        gate.onStateChange();
    }
}

TEST_CASE("Gate: skips unconfigured modules", "[Gate]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    StateMachine stateMachine{};

    // nothing stored in settings, so every module resolves to GPIO_NUM_NC pins
    // and is therefore unconfigured - none of them are attempted
    Gate gate(stateMachine, settings, pr, gpioStub, adcStub, randomStub, timeStub);

    SECTION("initializes successfully when no module is configured") {
        REQUIRE(gate.initialize());
        REQUIRE(gate.isReady());
    }

    SECTION("fixedUpdate and onStateChange don't crash with no configured modules") {
        REQUIRE(gate.initialize());
        gate.fixedUpdate();
        gate.onStateChange();
    }
}

TEST_CASE("Gate: initializes only the configured modules", "[Gate]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    StateMachine stateMachine{};

    // only module 0 is configured, the other 3 stay GPIO_NUM_NC
    REQUIRE(settings.storeGateModuleLaserGpioPin(0, GPIO_NUM_1));
    REQUIRE(settings.storeGateModuleLedGpioPin(0, GPIO_NUM_2));
    REQUIRE(settings.storeGateModuleLdrGpioPin(0, GPIO_NUM_34));

    Gate gate(stateMachine, settings, pr, gpioStub, adcStub, randomStub, timeStub);

    REQUIRE(gate.initialize());
    REQUIRE(gate.isReady());
    REQUIRE(pr.isPinBound(GPIO_NUM_1));
}
