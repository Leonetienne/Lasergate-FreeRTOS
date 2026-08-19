#include <catch2/catch_test_macros.hpp>
#include "Gate.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "SettingsManager.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/NVSStub.h"

TEST_CASE("Gate: lifecycle", "[Gate]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
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

    Gate gate(stateMachine, settings, pr, gpioStub, adcStub);

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

TEST_CASE("Gate: unconfigured pins fall back to GPIO_NUM_NC", "[Gate]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    StateMachine stateMachine{};

    // nothing stored in settings. every module's pins resolve to the same
    // GPIO_NUM_NC sentinel, which collide against each other on binding
    Gate gate(stateMachine, settings, pr, gpioStub, adcStub);

    REQUIRE_FALSE(gate.initialize());
}
