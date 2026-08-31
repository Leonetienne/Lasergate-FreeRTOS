#include <catch2/catch_test_macros.hpp>
#include "GateModule.h"
#include "GpioPinRegister.h"
#include "SettingsManager.h"
#include "StateMachine.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/SystemStub.h"
#include "test/stubs/TimeStub.h"
#include "LaserPulseFreqCalibConfig.h"
#include "LdrThreshCalibConfig.h"
#include "GateModuleConfig.h"

namespace {
    // sets the ADC reading to match whatever the laser diode is currently doing, then ticks a pulse
    void tickCleanPulse(SystemStub& stub, System& system, gpio_num_t laserPin, adc_channel_t ldrChannel, uint16_t threshold, uint16_t pulseFrequency) noexcept {
        const bool laserOn = stub.gpio.test_gpioGetLevel(laserPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        stub.adcOneshot.test_setChannelValue(ldrChannel, laserOn ? static_cast<uint16_t>(threshold + 1) : static_cast<uint16_t>(0));

        stub.time.setStubbedMillis(stub.time.getMillis() + pulseFrequency + 1);
        system.update();
    }

    // same, but flipped, so this tick registers as a misread
    void tickMisreadPulse(SystemStub& stub, System& system, gpio_num_t laserPin, adc_channel_t ldrChannel, uint16_t threshold, uint16_t pulseFrequency) noexcept {
        const bool laserOn = stub.gpio.test_gpioGetLevel(laserPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        stub.adcOneshot.test_setChannelValue(ldrChannel, laserOn ? static_cast<uint16_t>(0) : static_cast<uint16_t>(threshold + 1));

        stub.time.setStubbedMillis(stub.time.getMillis() + pulseFrequency + 1);
        system.update();
    }

    // same two, but against a bare GateModule (isPulseBatchAcceptable isn't reachable through Gate)
    void tickCleanPulse(GateModule& module, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, gpio_num_t laserPin, adc_channel_t ldrChannel, uint16_t threshold) noexcept {
        const bool laserOn = gpioStub.test_gpioGetLevel(laserPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        adcStub.test_setChannelValue(ldrChannel, laserOn ? static_cast<uint16_t>(threshold + 1) : static_cast<uint16_t>(0));

        timeStub.setStubbedMillis(timeStub.getMillis() + module.getPulseFrequency() + 1);
        module.fixedUpdate();
    }

    void tickMisreadPulse(GateModule& module, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, gpio_num_t laserPin, adc_channel_t ldrChannel, uint16_t threshold) noexcept {
        const bool laserOn = gpioStub.test_gpioGetLevel(laserPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        adcStub.test_setChannelValue(ldrChannel, laserOn ? static_cast<uint16_t>(0) : static_cast<uint16_t>(threshold + 1));

        timeStub.setStubbedMillis(timeStub.getMillis() + module.getPulseFrequency() + 1);
        module.fixedUpdate();
    }
}

TEST_CASE("GateModule: pulse frequency settings", "[GateModule]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("test"));
    SettingsManager settings(nvs);

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3

    SECTION("falls back to the max (most conservative) frequency when nothing is stored yet") {
        GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);
        REQUIRE(module.initialize());
        REQUIRE(module.getPulseFrequency() == CALIB_PULSE_FREQ_MAX_FREQ);
    }

    SECTION("loads a previously calibrated frequency for its own index") {
        REQUIRE(settings.storeGateModuleLaserPulseFrequency(0, 123));

        GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);
        REQUIRE(module.initialize());
        REQUIRE(module.getPulseFrequency() == 123);
    }

    SECTION("does not pick up a frequency stored under a different index") {
        REQUIRE(settings.storeGateModuleLaserPulseFrequency(1, 123));

        GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);
        REQUIRE(module.initialize());
        REQUIRE(module.getPulseFrequency() == CALIB_PULSE_FREQ_MAX_FREQ);
    }
}

TEST_CASE("GateModule: LDR threshold settings", "[GateModule]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("test"));
    SettingsManager settings(nvs);

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3

    SECTION("falls back to the initial default when nothing is stored yet") {
        GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);
        REQUIRE(module.initialize());
        REQUIRE(module.getLdrThreshold() == CALIB_LDR_THRESH_INITIAL_THRESH);
    }

    SECTION("loads a previously calibrated threshold for its own index") {
        REQUIRE(settings.storeGateModuleLdrThreshold(0, 2222));

        GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);
        REQUIRE(module.initialize());
        REQUIRE(module.getLdrThreshold() == 2222);
    }

    SECTION("does not pick up a threshold stored under a different index") {
        REQUIRE(settings.storeGateModuleLdrThreshold(1, 2222));

        GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);
        REQUIRE(module.initialize());
        REQUIRE(module.getLdrThreshold() == CALIB_LDR_THRESH_INITIAL_THRESH);
    }
}

TEST_CASE("GateModule: lifecycle", "[GateModule]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("test"));
    SettingsManager settings(nvs);

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);

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
    NVSStub nvs{};
    REQUIRE(nvs.begin("test"));
    SettingsManager settings(nvs);

    SECTION("true when the laser and ldr pins are both real, regardless of the status led") {
        GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, GPIO_NUM_16, GPIO_NUM_NC, GPIO_NUM_7);
        REQUIRE(module.isConfigured());
    }

    SECTION("false when the laser pin is GPIO_NUM_NC") {
        GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, GPIO_NUM_NC, GPIO_NUM_17, GPIO_NUM_7);
        REQUIRE_FALSE(module.isConfigured());
    }

    SECTION("false when the ldr pin is GPIO_NUM_NC") {
        GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_NC);
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
    NVSStub nvs{};
    REQUIRE(nvs.begin("test"));
    SettingsManager settings(nvs);

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ldrPin = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, GPIO_NUM_NC, ldrPin);

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
    NVSStub nvs{};
    REQUIRE(nvs.begin("test"));
    SettingsManager settings(nvs);

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);
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

TEST_CASE("GateModule: OBSERVING status led behaviour", "[GateModule]") {
    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3
    constexpr adc_channel_t ldrChannel = ADC_CHANNEL_6;

    // goes through System/Gate module 0 instead of a bare GateModule, so init wires
    // onStateChange and the adc stub for us
    SystemStub stub;
    REQUIRE(stub.settings.storeGateModuleLaserGpioPin(0, laserPin));
    REQUIRE(stub.settings.storeGateModuleLedGpioPin(0, ledPin));
    REQUIRE(stub.settings.storeGateModuleLdrGpioPin(0, ldrPin));

    System& system = stub.buildSystem();
    system.initialize();

    // module 0 never stores a calibrated threshold/frequency, so it falls back to these
    constexpr uint16_t threshold = CALIB_LDR_THRESH_INITIAL_THRESH;
    constexpr uint16_t pulseFrequency = CALIB_PULSE_FREQ_MAX_FREQ;

    stub.random.test_setSeed(1234);

    // USER_ADJUSTING_BEAMS leaves the laser on, so bounce through it first
    stub.stateMachine.setState(STATE::DISARMED);
    stub.stateMachine.setState(STATE::USER_ADJUSTING_BEAMS);
    stub.stateMachine.setState(STATE::DISARMED);
    stub.stateMachine.setState(STATE::OBSERVING);

    SECTION("turns the laser off and the status led on entry") {
        REQUIRE(stub.gpio.test_gpioGetLevel(laserPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("status led stays on through the warm-up window, before the ring buffer saturates") {
        for (int i = 0; i < 31; ++i) {
            tickCleanPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);
        }

        REQUIRE(stub.stateMachine.getState() == STATE::OBSERVING);
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("status led turns off once a full clean batch has been observed") {
        for (int i = 0; i < 32; ++i) {
            tickCleanPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);
        }

        REQUIRE(stub.stateMachine.getState() == STATE::OBSERVING);
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("status led turns back on immediately when the latest pulse misreads") {
        for (int i = 0; i < 32; ++i) {
            tickCleanPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);
        }
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        tickMisreadPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);

        REQUIRE(stub.stateMachine.getState() == STATE::OBSERVING);
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("status led goes back off on the next clean pulse after a misread blip") {
        for (int i = 0; i < 32; ++i) {
            tickCleanPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);
        }
        tickMisreadPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));

        tickCleanPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);

        REQUIRE(stub.stateMachine.getState() == STATE::OBSERVING);
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("re-entering OBSERVING resets the warm-up window, even after saturating once") {
        for (int i = 0; i < 32; ++i) {
            tickCleanPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);
        }
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        stub.stateMachine.setState(STATE::DISARMED);
        stub.stateMachine.setState(STATE::OBSERVING);
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));

        // one clean pulse isn't enough to re-saturate the buffer
        tickCleanPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }
}

TEST_CASE("GateModule: OBSERVING status led is optional", "[GateModule]") {
    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ldrPin = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3
    constexpr adc_channel_t ldrChannel = ADC_CHANNEL_6;

    SystemStub stub;
    REQUIRE(stub.settings.storeGateModuleLaserGpioPin(0, laserPin));
    REQUIRE(stub.settings.storeGateModuleLdrGpioPin(0, ldrPin));
    // no led pin stored, stays GPIO_NUM_NC / unconfigured

    System& system = stub.buildSystem();
    system.initialize();

    constexpr uint16_t threshold = CALIB_LDR_THRESH_INITIAL_THRESH;
    constexpr uint16_t pulseFrequency = CALIB_PULSE_FREQ_MAX_FREQ;

    stub.random.test_setSeed(1234);

    stub.stateMachine.setState(STATE::DISARMED);
    stub.stateMachine.setState(STATE::OBSERVING);

    // status led has no pin, this shouldn't touch or fault on it
    for (int i = 0; i < 40; ++i) {
        tickCleanPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);
    }
    tickMisreadPulse(stub, system, laserPin, ldrChannel, threshold, pulseFrequency);

    REQUIRE(stub.stateMachine.getState() == STATE::OBSERVING);
    REQUIRE_FALSE(stub.gpioPinRegister.isPinBound(GPIO_NUM_NC));
}

TEST_CASE("GateModule: ALARM status led behaviour", "[GateModule]") {
    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3

    // goes through System/Gate module 0 instead of a bare GateModule, so init wires
    // onStateChange and the adc stub for us
    SystemStub stub;
    REQUIRE(stub.settings.storeGateModuleLaserGpioPin(0, laserPin));
    REQUIRE(stub.settings.storeGateModuleLedGpioPin(0, ledPin));
    REQUIRE(stub.settings.storeGateModuleLdrGpioPin(0, ldrPin));

    System& system = stub.buildSystem();
    system.initialize();

    // ALARM is only reachable from OBSERVING
    stub.stateMachine.setState(STATE::DISARMED);
    stub.stateMachine.setState(STATE::OBSERVING);

    SECTION("turns the status led on upon entering ALARM") {
        stub.stateMachine.setState(STATE::ALARM);

        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("status led stays on before the blink interval elapses") {
        stub.stateMachine.setState(STATE::ALARM);

        stub.time.setStubbedMillis(stub.time.getMillis() + ALARM_STATE_STATUS_LED_BLINK_INTERVAL - 1);
        system.update();

        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("status led turns off once the blink interval elapses") {
        stub.stateMachine.setState(STATE::ALARM);

        stub.time.setStubbedMillis(stub.time.getMillis() + ALARM_STATE_STATUS_LED_BLINK_INTERVAL + 1);
        system.update();

        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("status led blinks back on after a second interval") {
        stub.stateMachine.setState(STATE::ALARM);

        stub.time.setStubbedMillis(stub.time.getMillis() + ALARM_STATE_STATUS_LED_BLINK_INTERVAL + 1);
        system.update();
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

        stub.time.setStubbedMillis(stub.time.getMillis() + ALARM_STATE_STATUS_LED_BLINK_INTERVAL + 1);
        system.update();
        REQUIRE(stub.gpio.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }
}

TEST_CASE("GateModule: ALARM status led is optional", "[GateModule]") {
    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ldrPin = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3

    SystemStub stub;
    REQUIRE(stub.settings.storeGateModuleLaserGpioPin(0, laserPin));
    REQUIRE(stub.settings.storeGateModuleLdrGpioPin(0, ldrPin));
    // no led pin stored, stays GPIO_NUM_NC / unconfigured

    System& system = stub.buildSystem();
    system.initialize();

    // ALARM is only reachable from OBSERVING
    stub.stateMachine.setState(STATE::DISARMED);
    stub.stateMachine.setState(STATE::OBSERVING);
    stub.stateMachine.setState(STATE::ALARM);

    // status led has no pin, this shouldn't touch or fault on it across several blink cycles
    for (int i = 0; i < 5; ++i) {
        stub.time.setStubbedMillis(stub.time.getMillis() + ALARM_STATE_STATUS_LED_BLINK_INTERVAL + 1);
        system.update();
    }

    REQUIRE(stub.stateMachine.getState() == STATE::ALARM);
    REQUIRE_FALSE(stub.gpioPinRegister.isPinBound(GPIO_NUM_NC));
}

TEST_CASE("GateModule: isPulseBatchAcceptable", "[GateModule]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    REQUIRE(adcStub.initialize() == ESP_OK);
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("test"));
    SettingsManager settings(nvs);

    constexpr gpio_num_t laserPin = GPIO_NUM_16;
    constexpr gpio_num_t ledPin = GPIO_NUM_17;
    constexpr gpio_num_t ldrPin = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3
    constexpr adc_channel_t ldrChannel = ADC_CHANNEL_6;

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, laserPin, ledPin, ldrPin);
    REQUIRE(module.initialize());
    const uint16_t threshold = module.getLdrThreshold();

    randomStub.test_setSeed(1234);

    stateMachine.setState(STATE::DISARMED);
    module.onStateChange();
    stateMachine.setState(STATE::OBSERVING);
    module.onStateChange();

    SECTION("stays unresolved before 30 clean readings saturate the batch") {
        for (int i = 0; i < 30; ++i) {
            tickCleanPulse(module, gpioStub, adcStub, timeStub, laserPin, ldrChannel, threshold);
        }

        REQUIRE_FALSE(module.isPulseBatchAcceptable().has_value());
    }

    SECTION("clears once 32 clean readings saturate the batch") {
        for (int i = 0; i < 32; ++i) {
            tickCleanPulse(module, gpioStub, adcStub, timeStub, laserPin, ldrChannel, threshold);
        }

        REQUIRE(module.isPulseBatchAcceptable() == true);
    }

    SECTION("still clears with exactly the allowed number of misreads") {
        for (int i = 0; i < ALLOWED_MISREADS_PER_BATCH; ++i) {
            tickMisreadPulse(module, gpioStub, adcStub, timeStub, laserPin, ldrChannel, threshold);
        }
        for (int i = 0; i < 32 - ALLOWED_MISREADS_PER_BATCH; ++i) {
            tickCleanPulse(module, gpioStub, adcStub, timeStub, laserPin, ldrChannel, threshold);
        }

        REQUIRE(module.isPulseBatchAcceptable() == true);
    }

    SECTION("rejects the batch once more than the allowed misreads happen") {
        for (int i = 0; i < ALLOWED_MISREADS_PER_BATCH + 1; ++i) {
            tickMisreadPulse(module, gpioStub, adcStub, timeStub, laserPin, ldrChannel, threshold);
        }
        for (int i = 0; i < 31 - ALLOWED_MISREADS_PER_BATCH; ++i) {
            tickCleanPulse(module, gpioStub, adcStub, timeStub, laserPin, ldrChannel, threshold);
        }

        REQUIRE(module.isPulseBatchAcceptable() == false);
    }

    SECTION("clears again once the misreads roll out of the window") {
        for (int i = 0; i < ALLOWED_MISREADS_PER_BATCH + 1; ++i) {
            tickMisreadPulse(module, gpioStub, adcStub, timeStub, laserPin, ldrChannel, threshold);
        }
        for (int i = 0; i < 31 - ALLOWED_MISREADS_PER_BATCH; ++i) {
            tickCleanPulse(module, gpioStub, adcStub, timeStub, laserPin, ldrChannel, threshold);
        }
        REQUIRE(module.isPulseBatchAcceptable() == false);

        // ring buffer is 32 wide, so 32 more clean pulses fully push the misreads out
        for (int i = 0; i < 32; ++i) {
            tickCleanPulse(module, gpioStub, adcStub, timeStub, laserPin, ldrChannel, threshold);
        }

        REQUIRE(module.isPulseBatchAcceptable() == true);
    }
}
