#include <catch2/catch_test_macros.hpp>
#include "GateModule.h"
#include "GpioPinRegister.h"
#include "SettingsManager.h"
#include "StateMachine.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/TimeStub.h"
#include "test/stubs/LdrPhysicsSim.h"
#include "LaserPulseFreqCalibConfig.h"

// separate file, same reasoning as LdrThreshCalibration.cpp: kept isolated so only the
// drivers below need to change if this logic moves out of GateModule later

namespace {
    constexpr gpio_num_t LASER_PIN = GPIO_NUM_16;
    constexpr gpio_num_t LED_PIN = GPIO_NUM_17;
    constexpr gpio_num_t LDR_PIN = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3
    constexpr adc_channel_t LDR_CHANNEL = ADC_CHANNEL_6;

    // one pulse cycle, timed off the module's *current* pulse frequency (it shrinks as
    // calibration homes in, unlike the fixed period LdrThreshCalibration uses)
    void tickPulse(GateModule& module, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, LdrPhysicsSim& ldrSim) noexcept {
        const bool laserOn = gpioStub.test_gpioGetLevel(LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        ldrSim.setPowerState(laserOn, timeStub.getMillis());

        const int64_t stepMillis = static_cast<int64_t>(module.getPulseFrequency()) + 1; // just past the current period
        const int64_t now = timeStub.getMillis() + stepMillis;
        timeStub.setStubbedMillis(now);
        adcStub.test_setChannelValue(LDR_CHANNEL, ldrSim.getCurrentReading(now));

        module.fixedUpdate();
    }

    void runPulses(GateModule& module, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, LdrPhysicsSim& ldrSim, int count) noexcept {
        for (int i = 0; i < count; ++i) {
            tickPulse(module, gpioStub, adcStub, timeStub, ldrSim);
        }
    }

    void enterCalibration(GateModule& module, StateMachine& stateMachine) noexcept {
        stateMachine.setState(STATE::DISARMED);
        module.onStateChange();
        stateMachine.setState(STATE::CALIBRATION_MODULATION_FREQUENCY);
        module.onStateChange();
    }
}

TEST_CASE("laser pulse frequency calibration: converges on the fastest frequency the ldr can keep up with", "[LaserPulseFreqCalibration]") {
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

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, LASER_PIN, LED_PIN, LDR_PIN);
    REQUIRE(module.initialize());

    // ambient/lit stay well clear of the (default) ldr threshold. this test is only about
    // whether the pulse period leaves the ldr time to settle, not about the threshold itself.
    // 300ms rise/fall sits comfortably inside [MIN_FREQ, MAX_FREQ] = [50, 800]ms, so homing
    // down from the conservative 800ms start should find a real boundary before hitting the
    // 50ms floor
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(800, 3900, 300);

    enterCalibration(module, stateMachine);

    bool reachedTerminalState = false;
    for (int batch = 0; batch < 60 && !reachedTerminalState; ++batch) {
        runPulses(module, gpioStub, adcStub, timeStub, ldrSim, 32);
        REQUIRE(stateMachine.getState() != STATE::FAULT);
        if (stateMachine.getState() == STATE::DISARMED) {
            reachedTerminalState = true;
        }
    }

    REQUIRE(reachedTerminalState);

    // it should've actually sped up, and stayed within the configured range
    const uint16_t calibratedFrequency = module.getPulseFrequency();
    REQUIRE(calibratedFrequency < CALIB_PULSE_FREQ_MAX_FREQ);
    REQUIRE(calibratedFrequency >= CALIB_PULSE_FREQ_MIN_FREQ);
}

TEST_CASE("laser pulse frequency calibration: does not hang forever", "[LaserPulseFreqCalibration]") {
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

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, LASER_PIN, LED_PIN, LDR_PIN);
    REQUIRE(module.initialize());

    // rise/fall time so fast it settles comfortably even at the 50ms floor. homing down
    // should never actually produce a bad batch, all the way to CALIB_PULSE_FREQ_MIN_FREQ
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(800, 3900, 5);

    enterCalibration(module, stateMachine);

    bool reachedTerminalState = false;
    for (int batch = 0; batch < 60 && !reachedTerminalState; ++batch) {
        runPulses(module, gpioStub, adcStub, timeStub, ldrSim, 32);
        REQUIRE(stateMachine.getState() != STATE::FAULT);
        if (stateMachine.getState() == STATE::DISARMED) {
            reachedTerminalState = true;
        }
    }

    // if the signal never fails, calibration must still conclude once it hits the floor -
    // not loop at CALIB_PULSE_FREQ_MIN_FREQ forever
    REQUIRE(reachedTerminalState);
}

TEST_CASE("laser pulse frequency calibration: faults when even the slowest frequency already misreads", "[LaserPulseFreqCalibration]") {
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

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, LASER_PIN, LED_PIN, LDR_PIN);
    REQUIRE(module.initialize());

    // rise/fall time (2000ms) far outlasts even the slowest, most conservative calibration
    // period (MAX_FREQ = 800ms). the very first batch can't keep up, so there's never a
    // known-good frequency to fall back to
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(800, 3900, 2000);

    enterCalibration(module, stateMachine);
    runPulses(module, gpioStub, adcStub, timeStub, ldrSim, 32);

    REQUIRE(stateMachine.getState() == STATE::FAULT);
    REQUIRE(stateMachine.getLastFaultReason().find("calibration failed") != std::string::npos);
}

TEST_CASE("laser pulse frequency calibration: resets to the max (safest) frequency on re-entry", "[LaserPulseFreqCalibration]") {
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

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, LASER_PIN, LED_PIN, LDR_PIN);
    REQUIRE(module.initialize());

    randomStub.test_setSeed(1234);
    enterCalibration(module, stateMachine);
    REQUIRE(module.getPulseFrequency() == CALIB_PULSE_FREQ_MAX_FREQ);

    // a clean, fast-settling signal lets one batch pass and the period shrink away from the default
    LdrPhysicsSim cleanSim(800, 3900, 10);
    runPulses(module, gpioStub, adcStub, timeStub, cleanSim, 32);
    REQUIRE(stateMachine.getState() != STATE::FAULT);
    REQUIRE(module.getPulseFrequency() < CALIB_PULSE_FREQ_MAX_FREQ);

    // bail out mid-calibration (e.g. a user cancel) and start over. enterCalibration()
    // itself already goes through DISARMED first, so no separate exit step is needed
    enterCalibration(module, stateMachine);

    REQUIRE(stateMachine.getState() == STATE::CALIBRATION_MODULATION_FREQUENCY);
    REQUIRE(module.getPulseFrequency() == CALIB_PULSE_FREQ_MAX_FREQ);
}
