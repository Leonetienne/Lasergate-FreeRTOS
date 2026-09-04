#include <catch2/catch_test_macros.hpp>
#include "Gate.h"
#include "GpioPinRegister.h"
#include "SettingsManager.h"
#include "StateMachine.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/LdrPhysicsSim.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/TimeStub.h"
#include "GateModuleConfig.h"
#include "PulseRingBuffer.h"

// Covers Gate's behaviour while driving DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST across modules.

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

    void enterDiagnosticSignalNoiseSelfTest(StateMachine& stateMachine, Gate& gate) noexcept {
        stateMachine.setState(STATE::DISARMED);
        gate.onStateChange();
        stateMachine.setState(STATE::DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST);
        gate.onStateChange();
    }

    // one shared-clock tick for both modules. neither module has a calibrated frequency stored,
    // so both fall back to CALIB_PULSE_FREQ_MAX_FREQ (800ms);
    void tickSignalPulse(Gate& gate, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, LdrPhysicsSim& sim0, LdrPhysicsSim& sim1) noexcept {
        const bool laserOn0 = gpioStub.test_gpioGetLevel(MODULE0_LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        const bool laserOn1 = gpioStub.test_gpioGetLevel(MODULE1_LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        sim0.setPowerState(laserOn0, timeStub.getMillis());
        sim1.setPowerState(laserOn1, timeStub.getMillis());

        constexpr int64_t PULSE_STEP_MILLIS = 801; // just past CALIB_PULSE_FREQ_MAX_FREQ (800ms)
        const int64_t now = timeStub.getMillis() + PULSE_STEP_MILLIS;
        timeStub.setStubbedMillis(now);
        adcStub.test_setChannelValue(MODULE0_LDR_CHANNEL, sim0.getCurrentReading(now));
        adcStub.test_setChannelValue(MODULE1_LDR_CHANNEL, sim1.getCurrentReading(now));

        gate.fixedUpdate();
    }
}

TEST_CASE("Gate: diagnostic signal noise self-test returns to DISARMED once all configured modules finish their batches", "[GateDiagnosticSignalNoiseSelfTest]") {
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
    Gate gate(stateMachine, settings, pr, gpioStub, adcStub, randomStub, timeStub);
    REQUIRE(gate.initialize());

    randomStub.test_setSeed(1234);
    // both modules: rise/fall time (150ms) settles well within the 800ms pulse frequency, so
    // pulses read cleanly and only the batch/tick bookkeeping drives when the self-test concludes
    LdrPhysicsSim sim0(100, 3900, 150);
    LdrPhysicsSim sim1(800, 3900, 150);

    enterDiagnosticSignalNoiseSelfTest(stateMachine, gate);
    REQUIRE(gpioStub.test_gpioGetLevel(MODULE0_LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    REQUIRE(gpioStub.test_gpioGetLevel(MODULE0_LED_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    REQUIRE(gpioStub.test_gpioGetLevel(MODULE1_LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    REQUIRE(gpioStub.test_gpioGetLevel(MODULE1_LED_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));

    // a full self-test is DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST_NUM_BATCHES batches of
    // PulseRingBuffer::getBufferSize() pulses each; both modules finish on the same tick here
    // since they share the same default pulse frequency, so this doesn't distinguish
    // "first module done" from "all modules done".
    constexpr int ticksForFullRun = static_cast<int>(DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST_NUM_BATCHES * PulseRingBuffer::getBufferSize());

    bool reachedTerminalState = false;
    for (int i = 0; i < ticksForFullRun + 10 && !reachedTerminalState; ++i) {
        tickSignalPulse(gate, gpioStub, adcStub, timeStub, sim0, sim1);
        REQUIRE(stateMachine.getState() != STATE::FAULT);
        if (stateMachine.getState() == STATE::DISARMED) {
            reachedTerminalState = true;
        }
    }

    REQUIRE(reachedTerminalState);
}

TEST_CASE("Gate: diagnostic signal noise self-test concludes immediately with zero configured modules", "[GateDiagnosticSignalNoiseSelfTest]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    RandomStub randomStub{};
    TimeStub timeStub{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("system"));
    SettingsManager settings(nvs);
    StateMachine stateMachine{};

    // nothing stored in settings: every module resolves to GPIO_NUM_NC and is unconfigured
    Gate gate(stateMachine, settings, pr, gpioStub, adcStub, randomStub, timeStub);
    REQUIRE(gate.initialize());

    enterDiagnosticSignalNoiseSelfTest(stateMachine, gate);
    gate.fixedUpdate();

    REQUIRE(stateMachine.getState() == STATE::DISARMED);
}

TEST_CASE("Gate: diagnostic signal noise self-test waits for every configured module before concluding", "[GateDiagnosticSignalNoiseSelfTest]") {
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
    // module0 pulses much faster than module1, so it finishes its 16 batches well before
    // module1 does. module1's stored frequency (1600ms) is chosen so it only manages a
    // successful pulse roughly every other tick of PULSE_STEP_MILLIS (801ms).
    // must be stored before Gate/GateModule::initialize() reads it below.
    REQUIRE(settings.storeGateModuleLaserPulseFrequency(1, 1600));

    Gate gate(stateMachine, settings, pr, gpioStub, adcStub, randomStub, timeStub);
    REQUIRE(gate.initialize());

    randomStub.test_setSeed(1234);
    LdrPhysicsSim sim0(100, 3900, 50);
    LdrPhysicsSim sim1(800, 3900, 50);

    enterDiagnosticSignalNoiseSelfTest(stateMachine, gate);

    constexpr int module0FinishesAtTick = static_cast<int>(DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST_NUM_BATCHES * PulseRingBuffer::getBufferSize());
    constexpr int module1FinishesAtTick = module0FinishesAtTick * 2;

    // run up to (but not including) the tick where module0 alone finishes: nowhere near done yet
    for (int i = 0; i < module0FinishesAtTick - 1; ++i) {
        tickSignalPulse(gate, gpioStub, adcStub, timeStub, sim0, sim1);
        REQUIRE(stateMachine.getState() == STATE::DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST);
    }

    // module0 finishes on this tick, but module1 has barely started: must not conclude
    tickSignalPulse(gate, gpioStub, adcStub, timeStub, sim0, sim1);
    REQUIRE(stateMachine.getState() == STATE::DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST);

    // run up to (but not including) the tick where module1 also finishes: the gate must keep
    // waiting on it the whole way, even though module0 has been done for a long time
    for (int i = module0FinishesAtTick; i < module1FinishesAtTick - 1; ++i) {
        tickSignalPulse(gate, gpioStub, adcStub, timeStub, sim0, sim1);
        REQUIRE(stateMachine.getState() == STATE::DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST);
    }

    // module1 finishes on this exact tick: only now may the gate conclude
    tickSignalPulse(gate, gpioStub, adcStub, timeStub, sim0, sim1);
    REQUIRE(stateMachine.getState() == STATE::DISARMED);
}
