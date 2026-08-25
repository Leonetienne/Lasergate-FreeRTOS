#ifndef LASERGATE_TESTS_GATEMODULE_H
#define LASERGATE_TESTS_GATEMODULE_H

#include "GpioPinRegister.h"
#include "LaserDiode.h"
#include "LightEmittingDiode.h"
#include "LaserSensor.h"
#include "PulseRingBuffer.h"
#include "SettingsManager.h"
#include "StateMachine.h"
#include "hal/IAdcOneshot.h"
#include "hal/IGpio.h"
#include "hal/IRandom.h"
#include "hal/ITime.h"
#include <cstddef>
#include <optional>
#include <utility>

/**
 * Aggregates and drives a status led, a laser diode and a laser sensor into a module
 * that is able to detect objects interrupting the laser
 */
class GateModule {
public:
    GateModule(
        StateMachine& stateMachine,
        SettingsManager& settings,
        std::size_t settingsIndex,
        GpioPinRegister& pinRegister,
        IGpio& i_gpio,
        IAdcOneshot& i_adcOneshot,
        IRandom& i_random,
        ITime& i_time,
        gpio_num_t laserPin,
        gpio_num_t statusLedPin,
        gpio_num_t ldrPin
    ) noexcept;
    GateModule(const GateModule&) = delete;
    GateModule(GateModule&&) = delete;
    GateModule& operator=(const GateModule&) = delete;
    GateModule& operator=(GateModule&&) = delete;
    ~GateModule() noexcept;

    /**
     * @return Whether actual GPIO pins are used instead of NC, for laser diode and laser sensor
     */
    [[nodiscard]] bool isConfigured() const noexcept;

    /**
    * Will initialize this module
    * @return Success state
    */
    bool initialize() noexcept;

    /**
     * Will free all resources acquired by this module
     * @return Success state
     */
    bool free() noexcept;

    /**
     * @return Whether this module is ready for operation
     */
    [[nodiscard]] bool isReady() const noexcept;

    /**
     * Call every 10ms
     */
    void fixedUpdate() noexcept;

    /**
     * Gets called right after system state changes
     */
    void onStateChange() noexcept;

    /**
     * @return The laser sensor's currently active LDR threshold
     */
    [[nodiscard]] uint16_t getLdrThreshold() const noexcept;

    /**
     * @return The laser currently active pulse frequency (ms delay)
     */
    [[nodiscard]] uint16_t getPulseFrequency() const noexcept;

private:
    /**
     * Gets called once after state engine switches to FAULT
     */
    void onStateFault() noexcept;

    /**
     * Gets called once after state engine switches to USER_ADJUSTING_BEAMS
     */
    void onStateUserAdjustingBeams() noexcept;

    /**
     * Gets called once after state engine switches to CALIBRATION_LDR_THRESH
     */
    void onStateCalibrationLdrThresh() noexcept;

    /**
    * Gets called once after state engine switches to CALIBRATION_MODULATION_FREQUENCY
    */
    void onStateCalibrationModulationFrequency() noexcept;

    /**
     * Gets called once after state engine switches to OBSERVING
     */
    void onStateObserving() noexcept;

    /**
     * Gets called once after state engine switches to ALARM
     */
    void onStateAlarm() noexcept;

    /**
     * Gets called once after state engine switches to DISARMED
     */
    void onStateDisarmed() noexcept;

    /**
     * Gets called by update during state FAULT
     */
    void updateStateFault() noexcept;

    /**
     * Gets called by update during state USER_ADJUSTING_BEAMS
     */
    void updateStateUserAdjustingBeams() noexcept;

    /**
     * Gets called by update during state CALIBRATION_LDR_THRESH
     */
    void updateStateCalibrationLdrThresh() noexcept;

    /**
    * Gets called once after state engine switches to CALIBRATION_MODULATION_FREQUENCY
    */
    void updateStateCalibrationModulationFrequency() noexcept;

    /**
     * Concludes LDR threshold calibration: computes the calibrated value from the homed
     * lower/upper bounds, applies and persists it, resets calibration state, and moves the
     * state machine to DISARMED (or FAULT on a persistence failure).
     */
    void wrapUpLdrThreshCalib() noexcept;

    /**
     * Concludes the HOMING_LOWER phase of LDR threshold calibration: stores the homed lower
     * bound, resets the sensor to the initial threshold, and hands off to HOMING_UPPER.
     */
    void advanceLdrThreshCalib() noexcept;

    /**
     * Concludes laser pulse frequency calibration: applies and persists the last known-good
     * frequency, resets calibration state, and moves the state machine to DISARMED (or FAULT
     * if no known-good frequency was ever found, or on a persistence failure).
     */
    void wrapUpPulseFreqCalib() noexcept;

    /**
     * Gets called by update during state OBSERVING
     */
    void updateStateObserving() noexcept;

    /**
     * Gets called by update during state ALARM
     */
    void updateStateAlarm() noexcept;

    /**
      * Gets called by update during state DISARMED
      */
    void updateStateDisarmed() noexcept;

    /**
     * Will set decide a new pulse laser state and apply it to the laser diode
     */
    void applyPulseTarget() noexcept;

    /**
     * Resets the current monotonic millis timer
     */
    void resetPulseTimer() noexcept;

    /**
     * Reads the sensor and the laser diode's actual state for a pulse verification
     * @return {sensorDetectedOn, laserActuallyOn}, or std::nullopt on a read failure (also faults the state machine)
     */
    [[nodiscard]] std::optional<std::pair<bool, bool>> readPulseState() const noexcept;

    /**
     * Will complete a whole pulse cycle: verify, record, set new state
     */
    void doPulseCycle() noexcept;

    /**
     * @return Whether pulseHistory holds a batch that's good enough to be considered healthy (Num of misreads within tolerance)
     */
    [[nodiscard]] std::optional<bool> isPulseBatchAcceptable() const noexcept;


    bool isInitialized = false;

    StateMachine& stateMachine;
    SettingsManager& settings;
    std::size_t settings_index;
    IRandom& i_random;
    ITime& i_time;
    LaserDiode laserDiode;
    LightEmittingDiode statusLed;
    LaserSensor laserSensor;

    time_t pulseTimer;
    PulseRingBuffer pulseHistory;
    uint16_t laserPulseFrequency = 0;

    /* LDR Thresh calibration helpers */
    uint16_t calib_ldr_lower_threshold = 0;
    uint16_t calib_ldr_upper_threshold = 0;
    uint16_t calib_ldr_last_good_threshold = 0;
    enum class CALIB_LDR_STATE {
        NONE,
        HOMING_LOWER,
        HOMING_UPPER
    } calib_ldr_state = CALIB_LDR_STATE::NONE;

    /* Laser pulse frequency calibration helpers */
    uint16_t calib_freq_last_good_freq = 0;
};


#endif //LASERGATE_TESTS_GATEMODULE_H
