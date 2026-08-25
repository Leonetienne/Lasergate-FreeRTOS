#include <catch2/catch_test_macros.hpp>
#include "GateModule.h"
#include "GpioPinRegister.h"
#include "LdrThreshCalibrator.h"
#include "SettingsManager.h"
#include "StateMachine.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/LdrPhysicsSim.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/TimeStub.h"
#include "LdrThreshCalibConfig.h"

// drives LdrThreshCalibrator directly against a real GateModule, no need to go through Gate
// for this. Gate's own aggregation behavior (waiting on all modules, faulting immediately,
// skipping unconfigured ones) gets tested in GateCalibration.cpp instead

namespace {
    constexpr gpio_num_t LASER_PIN = GPIO_NUM_16;
    constexpr gpio_num_t LED_PIN = GPIO_NUM_17;
    constexpr gpio_num_t LDR_PIN = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6, see AdcGpioMapping.cpp
    constexpr adc_channel_t LDR_CHANNEL = ADC_CHANNEL_6;
    constexpr int64_t PULSE_STEP_MILLIS = 501; // just past CALIB_LDR_TRESH_PULSE_FREQ (500ms)

    // one pulse cycle: mirror the laser's actual gpio state into the ldr sim, advance time,
    // push the ramped reading into the adc, then let the calibrator react
    void tickPulse(LdrThreshCalibrator& calibrator, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, LdrPhysicsSim& ldrSim) noexcept {
        const bool laserOn = gpioStub.test_gpioGetLevel(LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        ldrSim.setPowerState(laserOn, timeStub.getMillis());

        const int64_t now = timeStub.getMillis() + PULSE_STEP_MILLIS;
        timeStub.setStubbedMillis(now);
        adcStub.test_setChannelValue(LDR_CHANNEL, ldrSim.getCurrentReading(now));

        calibrator.fixedUpdate();
    }

    void runPulses(LdrThreshCalibrator& calibrator, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, LdrPhysicsSim& ldrSim, int count) noexcept {
        for (int i = 0; i < count; ++i) {
            tickPulse(calibrator, gpioStub, adcStub, timeStub, ldrSim);
        }
    }
}

TEST_CASE("LDR threshold calibration: converges on a physically plausible ambient/lit split", "[LdrThreshCalibration]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    REQUIRE(adcStub.initialize() == ESP_OK); // real code has to do this too, easy to forget
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("test"));
    SettingsManager settings(nvs);

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, LASER_PIN, LED_PIN, LDR_PIN);
    REQUIRE(module.initialize());
    LdrThreshCalibrator calibrator(module);

    // ambient ~800, laser hit ~3900 (both realistic for a 12-bit adc, max 4095), 150ms
    // rise/fall: well inside the 500ms pulse period so readings are always settled by the
    // time we verify
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(800, 3900, 150);

    calibrator.begin();

    // once both boundaries are found, the calibrator concludes. that's our signal calibration
    // is done
    bool reachedTerminalState = false;
    for (int batch = 0; batch < 60 && !reachedTerminalState; ++batch) {
        runPulses(calibrator, gpioStub, adcStub, timeStub, ldrSim, 32);
        REQUIRE(calibrator.status() != LdrThreshCalibrator::Status::FAILED);
        if (calibrator.status() == LdrThreshCalibrator::Status::CONCLUDED) {
            reachedTerminalState = true;
        }
    }

    REQUIRE(reachedTerminalState);

    // and it should've actually landed somewhere sane, not just stuck at the default
    const uint16_t calibratedThreshold = module.getLdrThreshold();
    REQUIRE(calibratedThreshold > 800);
    REQUIRE(calibratedThreshold < 3900);
    REQUIRE(calibratedThreshold != CALIB_LDR_THRESH_INITIAL_THRESH);

    // and it must have persisted, so a reboot doesn't lose it
    REQUIRE(settings.retrieveGateModuleLdrThreshold(0).value_or(0) == calibratedThreshold);
}

TEST_CASE("LDR threshold calibration: does not run forever", "[LdrThreshCalibration]") {
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
    LdrThreshCalibrator calibrator(module);

    // ambient is so dark that even CALIB_LDR_TRESH_MIN_THRESH (the homing floor) still
    // correctly discriminates it from a laser hit. HOMING_LOWER should never see a bad
    // batch, so it never gets a reason to hand off to HOMING_UPPER. (the upper side can't
    // get stuck the same way: CALIB_LDR_TRESH_MAX_THRESH is the real 12-bit adc ceiling, so
    // a reading can never exceed it and homing_upper always eventually fails there.)
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(100, 3900, 150);

    calibrator.begin();

    bool reachedTerminalState = false;
    for (int batch = 0; batch < 60 && !reachedTerminalState; ++batch) {
        runPulses(calibrator, gpioStub, adcStub, timeStub, ldrSim, 32);
        REQUIRE(calibrator.status() != LdrThreshCalibrator::Status::FAILED);
        if (calibrator.status() == LdrThreshCalibrator::Status::CONCLUDED) {
            reachedTerminalState = true;
        }
    }

    // if the lower boundary is never actually breached, calibration must still conclude
    // once it hits the CALIB_LDR_TRESH_MIN_THRESH floor. not loop there forever
    REQUIRE(reachedTerminalState);
}

TEST_CASE("LDR threshold calibration: faults immediately when ambient light already swamps the initial threshold", "[LdrThreshCalibration]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    REQUIRE(adcStub.initialize() == ESP_OK); // real code has to do this too, easy to forget
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("test"));
    SettingsManager settings(nvs);

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, LASER_PIN, LED_PIN, LDR_PIN);
    REQUIRE(module.initialize());
    LdrThreshCalibrator calibrator(module);

    // ambient already above the initial threshold, so it can't tell on from off even on the
    // first batch: no known-good value to fall back to, real failure
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(CALIB_LDR_THRESH_INITIAL_THRESH + 1000, 3900, 150);

    calibrator.begin();
    runPulses(calibrator, gpioStub, adcStub, timeStub, ldrSim, 32);

    REQUIRE(calibrator.status() == LdrThreshCalibrator::Status::FAILED);
    REQUIRE(calibrator.failureReason().find("calibration failed") != std::string::npos);
}

TEST_CASE("LDR threshold calibration: a rise time close to the pulse period causes misreads", "[LdrThreshCalibration]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    REQUIRE(adcStub.initialize() == ESP_OK); // real code has to do this too, easy to forget
    RandomStub randomStub{};
    TimeStub timeStub{};
    StateMachine stateMachine{};
    NVSStub nvs{};
    REQUIRE(nvs.begin("test"));
    SettingsManager settings(nvs);

    GateModule module(stateMachine, settings, 0, pr, gpioStub, adcStub, randomStub, timeStub, LASER_PIN, LED_PIN, LDR_PIN);
    REQUIRE(module.initialize());
    LdrThreshCalibrator calibrator(module);

    // same ambient/lit split as the happy path, but fall time (900ms) now outlasts the pulse
    // period, so readings land mid-ramp in both directions. some false positive, some false
    // negative. mixed shape, so it gets caught by the direction check instead of being
    // mistaken for a real boundary.
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(800, 3900, 900);

    calibrator.begin();
    runPulses(calibrator, gpioStub, adcStub, timeStub, ldrSim, 32);

    REQUIRE(calibrator.status() == LdrThreshCalibrator::Status::FAILED);
    REQUIRE(calibrator.failureReason().find("unexpected misread shape") != std::string::npos);
}
