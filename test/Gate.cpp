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

    // ldr pins must stay within GPIO_NUM_1-10 (ADC_UNIT_1, ch0-9 on esp32-s3) since all 4
    // modules share one AdcOneshotStub bound to ADC_UNIT_1; laser/led are plain digital pins
    // so they're kept clear of the ADC1/ADC2 ranges (GPIO_NUM_1-20) to avoid pin collisions
    REQUIRE(settings.storeGateModuleLaserGpioPin(0, GPIO_NUM_41));
    REQUIRE(settings.storeGateModuleLedGpioPin(0, GPIO_NUM_42));
    REQUIRE(settings.storeGateModuleLdrGpioPin(0, GPIO_NUM_1));
    REQUIRE(settings.storeGateModuleLaserGpioPin(1, GPIO_NUM_43));
    REQUIRE(settings.storeGateModuleLedGpioPin(1, GPIO_NUM_44));
    REQUIRE(settings.storeGateModuleLdrGpioPin(1, GPIO_NUM_2));
    REQUIRE(settings.storeGateModuleLaserGpioPin(2, GPIO_NUM_45));
    REQUIRE(settings.storeGateModuleLedGpioPin(2, GPIO_NUM_46));
    REQUIRE(settings.storeGateModuleLdrGpioPin(2, GPIO_NUM_3));
    REQUIRE(settings.storeGateModuleLaserGpioPin(3, GPIO_NUM_47));
    REQUIRE(settings.storeGateModuleLedGpioPin(3, GPIO_NUM_48));
    REQUIRE(settings.storeGateModuleLdrGpioPin(3, GPIO_NUM_4));

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
    REQUIRE(settings.storeGateModuleLaserGpioPin(0, GPIO_NUM_41));
    REQUIRE(settings.storeGateModuleLedGpioPin(0, GPIO_NUM_42));
    REQUIRE(settings.storeGateModuleLdrGpioPin(0, GPIO_NUM_1));

    Gate gate(stateMachine, settings, pr, gpioStub, adcStub, randomStub, timeStub);

    REQUIRE(gate.initialize());
    REQUIRE(gate.isReady());
    REQUIRE(pr.isPinBound(GPIO_NUM_41));
}
