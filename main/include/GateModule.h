#ifndef LASERGATE_TESTS_GATEMODULE_H
#define LASERGATE_TESTS_GATEMODULE_H

#include "GpioPinRegister.h"
#include "LaserDiode.h"
#include "LightEmittingDiode.h"
#include "LaserSensor.h"
#include "PulseRingBuffer.h"
#include "StateMachine.h"
#include "hal/IAdcOneshot.h"
#include "hal/IGpio.h"
#include "hal/IRandom.h"
#include "hal/ITime.h"
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
     * @return The laser sensor's currently active LDR threshold (as set by calibration, or the initial default)
     */
    [[nodiscard]] uint16_t getLdrThreshold() const noexcept;

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
    IRandom& i_random;
    ITime& i_time;
    LaserDiode laserDiode;
    LightEmittingDiode statusLed;
    LaserSensor laserSensor;

    time_t pulseTimer;
    bool isInitialPulse = true;
    PulseRingBuffer pulseHistory;

    /* LDR Thresh calibration helpers */
    uint16_t calib_ldr_lower_threshold = 0;
    uint16_t calib_ldr_upper_threshold = 0;
    uint16_t calib_ldr_last_good_threshold = 0;
    enum class CALIB_LDR_STATE {
        NONE,
        HOMING_LOWER,
        HOMING_UPPER
    } calib_ldr_state = CALIB_LDR_STATE::NONE;
};


#endif //LASERGATE_TESTS_GATEMODULE_H
