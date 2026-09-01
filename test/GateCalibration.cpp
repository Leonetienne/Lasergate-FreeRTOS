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
#include "LdrThreshCalibConfig.h"

// covers Gate's aggregation across modules. waiting for all of them, faulting immediately on
// the first failure, skipping unconfigured ones. the calibration math itself lives in
// LdrThreshCalibration.cpp/LaserPulseFreqCalibration.cpp

namespace {
    constexpr gpio_num_t MODULE0_LASER_PIN = GPIO_NUM_41;
    constexpr gpio_num_t MODULE0_LED_PIN = GPIO_NUM_42;
    constexpr gpio_num_t MODULE0_LDR_PIN = GPIO_NUM_1; // -> ADC_UNIT_1 / ADC_CHANNEL_0
    constexpr adc_channel_t MODULE0_LDR_CHANNEL = ADC_CHANNEL_0;

    constexpr gpio_num_t MODULE1_LASER_PIN = GPIO_NUM_43;
    constexpr gpio_num_t MODULE1_LED_PIN = GPIO_NUM_44;
    constexpr gpio_num_t MODULE1_LDR_PIN = GPIO_NUM_2; // -> ADC_UNIT_1 / ADC_CHANNEL_1
    constexpr adc_channel_t MODULE1_LDR_CHANNEL = ADC_CHANNEL_1;

    // only sets up modules 0 and 1 - 2 and 3 stay NC, so these tests double as coverage that
    // unconfigured modules don't block completion
    void configureTwoModules(SettingsManager& settings) noexcept {
        REQUIRE(settings.storeGateModuleLaserGpioPin(0, MODULE0_LASER_PIN));
        REQUIRE(settings.storeGateModuleLedGpioPin(0, MODULE0_LED_PIN));
        REQUIRE(settings.storeGateModuleLdrGpioPin(0, MODULE0_LDR_PIN));
        REQUIRE(settings.storeGateModuleLaserGpioPin(1, MODULE1_LASER_PIN));
        REQUIRE(settings.storeGateModuleLedGpioPin(1, MODULE1_LED_PIN));
        REQUIRE(settings.storeGateModuleLdrGpioPin(1, MODULE1_LDR_PIN));
    }

    void enterCalibration(StateMachine& stateMachine, Gate& gate, STATE calibrationState) noexcept {
        stateMachine.setState(STATE::DISARMED);
        gate.onStateChange();
        stateMachine.setState(calibrationState);
        gate.onStateChange();
    }

    // one shared-clock tick for both modules, past CALIB_LDR_TRESH_PULSE_FREQ (constant, so a
    // fixed step works fine here)
    void tickLdrPulse(Gate& gate, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, LdrPhysicsSim& sim0, LdrPhysicsSim& sim1) noexcept {
        const bool laserOn0 = gpioStub.test_gpioGetLevel(MODULE0_LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        const bool laserOn1 = gpioStub.test_gpioGetLevel(MODULE1_LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        sim0.setPowerState(laserOn0, timeStub.getMillis());
        sim1.setPowerState(laserOn1, timeStub.getMillis());

        constexpr int64_t PULSE_STEP_MILLIS = 501; // just past CALIB_LDR_TRESH_PULSE_FREQ (500ms)
        const int64_t now = timeStub.getMillis() + PULSE_STEP_MILLIS;
        timeStub.setStubbedMillis(now);
        adcStub.test_setChannelValue(MODULE0_LDR_CHANNEL, sim0.getCurrentReading(now));
        adcStub.test_setChannelValue(MODULE1_LDR_CHANNEL, sim1.getCurrentReading(now));

        gate.fixedUpdate();
    }

    // same idea but for frequency calibration, where the period isn't constant. just always
    // step past the max (800ms) so both modules fire every tick regardless of where they're at
    void tickFreqPulse(Gate& gate, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, LdrPhysicsSim& sim0, LdrPhysicsSim& sim1) noexcept {
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

TEST_CASE("Gate: LDR calibration waits for all configured modules before concluding", "[GateCalibration]") {
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
    // module0: converges slowly (homes all the way to the floor before handing off)
    LdrPhysicsSim sim0(100, 3900, 150);
    // module1: converges faster (hits a real boundary well before module0 does)
    LdrPhysicsSim sim1(800, 3900, 150);

    enterCalibration(stateMachine, gate, STATE::CALIBRATION_LDR_THRESH);

    // run past the point where module1 alone would have concluded (empirically ~160 ticks)
    // but well short of module0's completion (~207 ticks)
    for (int i = 0; i < 185; ++i) {
        tickLdrPulse(gate, gpioStub, adcStub, timeStub, sim0, sim1);
        REQUIRE(stateMachine.getState() != STATE::FAULT);
    }

    // module1 has persisted its own result the moment it concluded...
    REQUIRE(settings.retrieveGateModuleLdrThreshold(1).has_value());
    // ...but the shared state machine must not have moved on without module0
    REQUIRE(stateMachine.getState() == STATE::CALIBRATION_LDR_THRESH);
    REQUIRE_FALSE(settings.retrieveGateModuleLdrThreshold(0).has_value());

    // keep going until module0 also concludes
    bool reachedTerminalState = false;
    for (int i = 0; i < 300 && !reachedTerminalState; ++i) {
        tickLdrPulse(gate, gpioStub, adcStub, timeStub, sim0, sim1);
        REQUIRE(stateMachine.getState() != STATE::FAULT);
        if (stateMachine.getState() == STATE::DISARMED) {
            reachedTerminalState = true;
        }
    }

    REQUIRE(reachedTerminalState);
    REQUIRE(settings.retrieveGateModuleLdrThreshold(0).has_value());
    REQUIRE(settings.retrieveGateModuleLdrThreshold(1).has_value());
}

TEST_CASE("Gate: LDR calibration faults immediately on the first failure, without waiting for other modules", "[GateCalibration]") {
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
    // module0: ambient already swamps the initial threshold. fails on the very first batch
    LdrPhysicsSim sim0(CALIB_LDR_THRESH_INITIAL_THRESH + 1000, 3900, 150);
    // module1: healthy physics that would take hundreds of ticks to converge
    LdrPhysicsSim sim1(800, 3900, 150);

    enterCalibration(stateMachine, gate, STATE::CALIBRATION_LDR_THRESH);

    bool faulted = false;
    for (int i = 0; i < 100 && !faulted; ++i) {
        tickLdrPulse(gate, gpioStub, adcStub, timeStub, sim0, sim1);
        if (stateMachine.getState() == STATE::FAULT) {
            faulted = true;
        }
    }

    REQUIRE(faulted);
    REQUIRE(stateMachine.getLastFaultReason().find("module 0") != std::string::npos);
    // module1 was nowhere near concluding yet. Gate didn't wait around for it
    REQUIRE_FALSE(settings.retrieveGateModuleLdrThreshold(1).has_value());
}

TEST_CASE("Gate: LDR calibration concludes immediately with zero configured modules", "[GateCalibration]") {
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

    enterCalibration(stateMachine, gate, STATE::CALIBRATION_LDR_THRESH);
    gate.fixedUpdate();

    REQUIRE(stateMachine.getState() == STATE::DISARMED);
}

TEST_CASE("Gate: laser pulse frequency calibration faults immediately on the first failure, without waiting for other modules", "[GateCalibration]") {
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
    // module0: rise/fall time (2000ms) far outlasts even the slowest calibration period
    // (800ms). fails on the very first batch
    LdrPhysicsSim sim0(800, 3900, 2000);
    // module1: healthy physics that would take a while to converge
    LdrPhysicsSim sim1(800, 3900, 150);

    enterCalibration(stateMachine, gate, STATE::CALIBRATION_MODULATION_FREQUENCY);

    bool faulted = false;
    for (int i = 0; i < 100 && !faulted; ++i) {
        tickFreqPulse(gate, gpioStub, adcStub, timeStub, sim0, sim1);
        if (stateMachine.getState() == STATE::FAULT) {
            faulted = true;
        }
    }

    REQUIRE(faulted);
    REQUIRE(stateMachine.getLastFaultReason().find("module 0") != std::string::npos);
    // module1 was nowhere near concluding yet. Gate didn't wait around for it
    REQUIRE_FALSE(settings.retrieveGateModuleLaserPulseFrequency(1).has_value());
}

TEST_CASE("Gate: laser pulse frequency calibration concludes immediately with zero configured modules", "[GateCalibration]") {
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

    enterCalibration(stateMachine, gate, STATE::CALIBRATION_MODULATION_FREQUENCY);
    gate.fixedUpdate();

    REQUIRE(stateMachine.getState() == STATE::DISARMED);
}
