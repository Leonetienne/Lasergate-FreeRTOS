#include <catch2/catch_test_macros.hpp>
#include "GateModule.h"
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

// separate file on purpose. this calibration logic will move out of GateModule at
// some point, keep it isolated so only the drivers below need to change, not the test cases

namespace {
    constexpr gpio_num_t LASER_PIN = GPIO_NUM_16;
    constexpr gpio_num_t LED_PIN = GPIO_NUM_17;
    constexpr gpio_num_t LDR_PIN = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6, see AdcGpioMapping.cpp
    constexpr adc_channel_t LDR_CHANNEL = ADC_CHANNEL_6;
    constexpr int64_t PULSE_STEP_MILLIS = 501; // just past CALIB_LDR_TRESH_PULSE_FREQ (500ms)

    // one pulse cycle: mirror the laser's actual gpio state into the ldr sim, advance time,
    // push the ramped reading into the adc, then let the module react
    void tickPulse(GateModule& module, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, LdrPhysicsSim& ldrSim) noexcept {
        const bool laserOn = gpioStub.test_gpioGetLevel(LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        ldrSim.setPowerState(laserOn, timeStub.getMillis());

        const int64_t now = timeStub.getMillis() + PULSE_STEP_MILLIS;
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
        stateMachine.setState(STATE::CALIBRATION_LDR_THRESH);
        module.onStateChange();
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

    // ambient ~800, laser hit ~3900 (both realistic for a 12-bit adc, max 4095), 150ms
    // rise/fall: well inside the 500ms pulse period so readings are always settled by the
    // time we verify
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(800, 3900, 150);

    enterCalibration(module, stateMachine);

    // once both boundaries are found, GateModule drops back to DISARMED. that's our signal
    // calibration is done
    bool reachedTerminalState = false;
    for (int batch = 0; batch < 60 && !reachedTerminalState; ++batch) {
        runPulses(module, gpioStub, adcStub, timeStub, ldrSim, 32);
        REQUIRE(stateMachine.getState() != STATE::FAULT);
        if (stateMachine.getState() == STATE::DISARMED) {
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

    // ambient already above the initial threshold, so it can't tell on from off even on the
    // first batch: no known-good value to fall back to, real failure
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(CALIB_LDR_THRESH_INITIAL_THRESH + 1000, 3900, 150);

    enterCalibration(module, stateMachine);
    runPulses(module, gpioStub, adcStub, timeStub, ldrSim, 32);

    REQUIRE(stateMachine.getState() == STATE::FAULT);
    REQUIRE(stateMachine.getLastFaultReason().find("calibration failed") != std::string::npos);
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

    // same ambient/lit split as the happy path, but fall time (900ms) is now longer than the
    // pulse period. a reading right after turning the laser off hasn't dropped back below
    // threshold yet, so it reads as a false positive.
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(800, 3900, 900);

    enterCalibration(module, stateMachine);
    runPulses(module, gpioStub, adcStub, timeStub, ldrSim, 32);

    REQUIRE(stateMachine.getState() == STATE::FAULT);
    REQUIRE(stateMachine.getLastFaultReason().find("calibration failed") != std::string::npos);
}
