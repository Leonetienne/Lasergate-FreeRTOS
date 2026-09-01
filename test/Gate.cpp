#include <catch2/catch_test_macros.hpp>
#include "Gate.h"
#include "GateConfig.h"
#include "GpioPinRegister.h"
#include "StateMachine.h"
#include "SettingsManager.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/TimeStub.h"
#include "LaserPulseFreqCalibConfig.h"
#include "LdrThreshCalibConfig.h"

// this fixture assumes exactly 2 configured modules is enough to distinguish "N or fewer
// interrupted" from "more than N interrupted", which only holds while ALLOWED_SHORT_TERM_INTERRUPTED_GATES == 1
static_assert(ALLOWED_SHORT_TERM_INTERRUPTED_GATES == 1);

namespace {
    constexpr gpio_num_t MODULE0_LASER_PIN = GPIO_NUM_41;
    constexpr gpio_num_t MODULE0_LED_PIN = GPIO_NUM_42;
    constexpr gpio_num_t MODULE0_LDR_PIN = GPIO_NUM_1; // -> ADC_UNIT_1 / ADC_CHANNEL_0
    constexpr adc_channel_t MODULE0_LDR_CHANNEL = ADC_CHANNEL_0;

    constexpr gpio_num_t MODULE1_LASER_PIN = GPIO_NUM_43;
    constexpr gpio_num_t MODULE1_LED_PIN = GPIO_NUM_44;
    constexpr gpio_num_t MODULE1_LDR_PIN = GPIO_NUM_2; // -> ADC_UNIT_1 / ADC_CHANNEL_1
    constexpr adc_channel_t MODULE1_LDR_CHANNEL = ADC_CHANNEL_1;

    void configureTwoModules(SettingsManager& settings) noexcept {
        REQUIRE(settings.storeGateModuleLaserGpioPin(0, MODULE0_LASER_PIN));
        REQUIRE(settings.storeGateModuleLedGpioPin(0, MODULE0_LED_PIN));
        REQUIRE(settings.storeGateModuleLdrGpioPin(0, MODULE0_LDR_PIN));
        REQUIRE(settings.storeGateModuleLaserGpioPin(1, MODULE1_LASER_PIN));
        REQUIRE(settings.storeGateModuleLedGpioPin(1, MODULE1_LED_PIN));
        REQUIRE(settings.storeGateModuleLdrGpioPin(1, MODULE1_LDR_PIN));
    }

    void enterObserving(StateMachine& stateMachine, Gate& gate) noexcept {
        stateMachine.setState(STATE::DISARMED);
        gate.onStateChange();
        stateMachine.setState(STATE::OBSERVING);
        gate.onStateChange();
    }

    // forces each module's ldr reading to match its actual current laser state (a clean pulse)
    // or mismatch it (a misread), then advances one shared pulse period and ticks the gate.
    // the step is pinned to CALIB_PULSE_FREQ_MAX_FREQ so both modules fire every tick
    // regardless of their own configured pulse frequency
    void tickObservingPulse(Gate& gate, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, bool misread0, bool misread1) noexcept {
        const bool laserOn0 = gpioStub.test_gpioGetLevel(MODULE0_LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        const bool laserOn1 = gpioStub.test_gpioGetLevel(MODULE1_LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        const bool wantsHigh0 = misread0 ? !laserOn0 : laserOn0;
        const bool wantsHigh1 = misread1 ? !laserOn1 : laserOn1;
        adcStub.test_setChannelValue(MODULE0_LDR_CHANNEL, wantsHigh0 ? static_cast<uint16_t>(CALIB_LDR_THRESH_INITIAL_THRESH + 1) : static_cast<uint16_t>(0));
        adcStub.test_setChannelValue(MODULE1_LDR_CHANNEL, wantsHigh1 ? static_cast<uint16_t>(CALIB_LDR_THRESH_INITIAL_THRESH + 1) : static_cast<uint16_t>(0));

        timeStub.setStubbedMillis(timeStub.getMillis() + CALIB_PULSE_FREQ_MAX_FREQ + 1);
        gate.fixedUpdate();
    }
}

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

TEST_CASE("Gate: intrusion detection", "[Gate]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    REQUIRE(adcStub.initialize() == ESP_OK);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    StateMachine stateMachine{};

    // modules 2 and 3 stay unconfigured throughout, which also covers that they never
    // count toward the interrupted tally
    configureTwoModules(settings);
    Gate gate(stateMachine, settings, pr, gpioStub, adcStub, randomStub, timeStub);
    REQUIRE(gate.initialize());

    randomStub.test_setSeed(1234);
    enterObserving(stateMachine, gate);

    const auto bufferSize = PulseRingBuffer::getBufferSize();

    SECTION("no modules interrupted: stays OBSERVING") {
        for (std::size_t i = 0; i < bufferSize * 3; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, false, false);
            REQUIRE(stateMachine.getState() == STATE::OBSERVING);
        }
    }

    SECTION("warm-up: interruptions before the batch saturates don't alarm") {
        for (std::size_t i = 0; i < bufferSize - 1; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, true);
            REQUIRE(stateMachine.getState() == STATE::OBSERVING);
        }
    }

    SECTION("more than the allowed number of modules interrupted: alarms immediately") {
        for (std::size_t i = 0; i < bufferSize - 1; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, true);
            REQUIRE(stateMachine.getState() == STATE::OBSERVING);
        }

        // the batch saturates on this tick: both modules interrupted at once
        tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, true);
        REQUIRE(stateMachine.getState() == STATE::ALARM);
    }

    SECTION("exactly one module interrupted: recovers before the grace period expires") {
        // clean warm-up so module 0's batch has a real (acceptable) verdict to interrupt
        for (std::size_t i = 0; i < bufferSize; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, false, false);
        }

        // 2 misreads is enough to push module 0's rolling window past the tolerance
        tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
        tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
        REQUIRE(stateMachine.getState() == STATE::OBSERVING);

        // still interrupted a bit longer, well short of a full batch's worth of grace time
        for (int i = 0; i < 2; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
            REQUIRE(stateMachine.getState() == STATE::OBSERVING);
        }

        // clears: a full batch of clean reads flushes the misreads out of the window
        for (std::size_t i = 0; i < bufferSize; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, false, false);
            REQUIRE(stateMachine.getState() == STATE::OBSERVING);
        }

        // keeps running clean well past where the original grace period would have expired
        for (std::size_t i = 0; i < bufferSize * 2; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, false, false);
            REQUIRE(stateMachine.getState() == STATE::OBSERVING);
        }
    }

    SECTION("exactly one module interrupted: alarms once it outlasts the grace period") {
        for (std::size_t i = 0; i < bufferSize; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, false, false);
        }

        // one full batch of continuous misreads is comfortably inside the grace period
        for (std::size_t i = 0; i < bufferSize; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
            REQUIRE(stateMachine.getState() == STATE::OBSERVING);
        }

        // keep going until it outlasts the grace period
        bool alarmed = false;
        for (std::size_t i = 0; i < bufferSize * 10 && !alarmed; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
            if (stateMachine.getState() == STATE::ALARM) {
                alarmed = true;
            }
        }
        REQUIRE(alarmed);
    }

    SECTION("re-entering OBSERVING never alarms off a stale grace period") {
        for (std::size_t i = 0; i < bufferSize; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, false, false);
        }

        // get partway into a grace period, but nowhere near expiring it
        tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
        tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
        REQUIRE(stateMachine.getState() == STATE::OBSERVING);

        // bail out and re-arm, same as a user disarming and re-arming mid-alarm-condition
        stateMachine.setState(STATE::DISARMED);
        gate.onStateChange();
        stateMachine.setState(STATE::OBSERVING);
        gate.onStateChange();

        // re-trigger an interruption in the fresh session, running through its own warm-up
        // phase first. the fresh session must not alarm off the earlier grace period
        for (std::size_t i = 0; i < bufferSize; ++i) {
            tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
        }
        REQUIRE(stateMachine.getState() == STATE::OBSERVING);
    }
}

TEST_CASE("Gate: intrusion detection grace period tracks the slowest configured module", "[Gate]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    REQUIRE(adcStub.initialize() == ESP_OK);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    StateMachine stateMachine{};

    configureTwoModules(settings);
    // module 0 pulses much faster than module 1, so the grace period must be governed by
    // module 1's slower batch time, not module 0's
    REQUIRE(settings.storeGateModuleLaserPulseFrequency(0, 200));
    REQUIRE(settings.storeGateModuleLaserPulseFrequency(1, 600));

    Gate gate(stateMachine, settings, pr, gpioStub, adcStub, randomStub, timeStub);
    REQUIRE(gate.initialize());

    randomStub.test_setSeed(1234);
    enterObserving(stateMachine, gate);

    constexpr auto bufferSize = PulseRingBuffer::getBufferSize();

    // interrupt module 0 continuously. ticks always advance by CALIB_PULSE_FREQ_MAX_FREQ, so
    // both modules still fire every tick regardless of their own configured frequency
    for (std::size_t i = 0; i < bufferSize; ++i) {
        tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
    }

    // elapsed time by here already exceeds what module 0's own (faster) batch time would
    // have allowed, but stays well short of module 1's (slower) one
    for (std::size_t i = 0; i < bufferSize / 2; ++i) {
        tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
        REQUIRE(stateMachine.getState() == STATE::OBSERVING);
    }

    // it must still alarm eventually, once it outlasts module 1's slower grace period
    bool alarmed = false;
    for (std::size_t i = 0; i < bufferSize * 10 && !alarmed; ++i) {
        tickObservingPulse(gate, gpioStub, adcStub, timeStub, true, false);
        if (stateMachine.getState() == STATE::ALARM) {
            alarmed = true;
        }
    }
    REQUIRE(alarmed);
}
