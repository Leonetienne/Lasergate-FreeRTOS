#include <catch2/catch_test_macros.hpp>
#include "GateModule.h"
#include "GpioPinRegister.h"
#include "PulseFreqCalibrator.h"
#include "SettingsManager.h"
#include "StateMachine.h"
#include "test/stubs/GpioStub.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/NVSStub.h"
#include "test/stubs/RandomStub.h"
#include "test/stubs/TimeStub.h"
#include "test/stubs/LdrPhysicsSim.h"
#include "LaserPulseFreqCalibConfig.h"

namespace {
    constexpr gpio_num_t LASER_PIN = GPIO_NUM_16;
    constexpr gpio_num_t LED_PIN = GPIO_NUM_17;
    constexpr gpio_num_t LDR_PIN = GPIO_NUM_7; // -> ADC_UNIT_1 / ADC_CHANNEL_6 on esp32-s3
    constexpr adc_channel_t LDR_CHANNEL = ADC_CHANNEL_6;

    // one pulse cycle, timed off the module's *current* pulse frequency (it shrinks as
    // calibration homes in, unlike the fixed period LdrThreshCalibration uses)
    void tickPulse(GateModule& module, PulseFreqCalibrator& calibrator, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, LdrPhysicsSim& ldrSim) noexcept {
        const bool laserOn = gpioStub.test_gpioGetLevel(LASER_PIN) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH);
        ldrSim.setPowerState(laserOn, timeStub.getMillis());

        const int64_t stepMillis = static_cast<int64_t>(module.getPulseFrequency()) + 1; // just past the current period
        const int64_t now = timeStub.getMillis() + stepMillis;
        timeStub.setStubbedMillis(now);
        adcStub.test_setChannelValue(LDR_CHANNEL, ldrSim.getCurrentReading(now));

        calibrator.fixedUpdate();
    }

    void runPulses(GateModule& module, PulseFreqCalibrator& calibrator, GpioStub& gpioStub, AdcOneshotStub& adcStub, TimeStub& timeStub, LdrPhysicsSim& ldrSim, int count) noexcept {
        for (int i = 0; i < count; ++i) {
            tickPulse(module, calibrator, gpioStub, adcStub, timeStub, ldrSim);
        }
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
    PulseFreqCalibrator calibrator(module);

    // ambient/lit stay well clear of the (default) ldr threshold. this test is only about
    // whether the pulse period leaves the ldr time to settle, not about the threshold itself.
    // 300ms rise/fall sits comfortably inside [MIN_FREQ, MAX_FREQ] = [50, 800]ms, so homing
    // down from the conservative 800ms start should find a real boundary before hitting the
    // 50ms floor
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(800, 3900, 300);

    calibrator.begin();

    bool reachedTerminalState = false;
    for (int batch = 0; batch < 60 && !reachedTerminalState; ++batch) {
        runPulses(module, calibrator, gpioStub, adcStub, timeStub, ldrSim, static_cast<int>(PulseRingBuffer::getBufferSize()));
        REQUIRE(calibrator.status() != PulseFreqCalibrator::Status::FAILED);
        if (calibrator.status() == PulseFreqCalibrator::Status::CONCLUDED) {
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
    PulseFreqCalibrator calibrator(module);

    // rise/fall time so fast it settles comfortably even at the 50ms floor. homing down
    // should never actually produce a bad batch, all the way to CALIB_PULSE_FREQ_MIN_FREQ
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(800, 3900, 5);

    calibrator.begin();

    bool reachedTerminalState = false;
    for (int batch = 0; batch < 60 && !reachedTerminalState; ++batch) {
        runPulses(module, calibrator, gpioStub, adcStub, timeStub, ldrSim, static_cast<int>(PulseRingBuffer::getBufferSize()));
        REQUIRE(calibrator.status() != PulseFreqCalibrator::Status::FAILED);
        if (calibrator.status() == PulseFreqCalibrator::Status::CONCLUDED) {
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
    PulseFreqCalibrator calibrator(module);

    // rise/fall time so slow that not even getBufferSize() consecutive same-state pulses cross the threshold. every pulse in the first batch misreads, so there's no known-good
    // frequency to fall back to
    randomStub.test_setSeed(1234);
    LdrPhysicsSim ldrSim(800, 3900, 1000000);

    calibrator.begin();
    runPulses(module, calibrator, gpioStub, adcStub, timeStub, ldrSim, static_cast<int>(PulseRingBuffer::getBufferSize()));

    REQUIRE(calibrator.status() == PulseFreqCalibrator::Status::FAILED);
    REQUIRE(calibrator.failureReason().find("calibration failed") != std::string::npos);
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
    PulseFreqCalibrator calibrator(module);

    randomStub.test_setSeed(1234);
    calibrator.begin();
    REQUIRE(module.getPulseFrequency() == CALIB_PULSE_FREQ_MAX_FREQ);

    // a clean, fast-settling signal lets one batch pass and the period shrink away from the default
    LdrPhysicsSim cleanSim(800, 3900, 10);
    runPulses(module, calibrator, gpioStub, adcStub, timeStub, cleanSim, static_cast<int>(PulseRingBuffer::getBufferSize()));
    REQUIRE(calibrator.status() != PulseFreqCalibrator::Status::FAILED);
    REQUIRE(module.getPulseFrequency() < CALIB_PULSE_FREQ_MAX_FREQ);

    // bail out mid-calibration (e.g. a user cancel) and start over
    calibrator.begin();

    REQUIRE(calibrator.status() == PulseFreqCalibrator::Status::RUNNING);
    REQUIRE(module.getPulseFrequency() == CALIB_PULSE_FREQ_MAX_FREQ);
}
