#ifndef LASERGATE_TESTS_GATEMODULE_H
#define LASERGATE_TESTS_GATEMODULE_H

#include "GpioPinRegister.h"
#include "LaserDiode.h"
#include "LightEmittingDiode.h"
#include "LaserSensor.h"
#include "StateMachine.h"
#include "hal/IAdcOneshot.h"
#include "hal/IGpio.h"
#include "hal/IRandom.h"
#include "hal/ITime.h"

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
     * Resets pulseRingPuffer, pulseRingPufferPointer, and pulseSampleCount
     */
    void resetPulseStats() noexcept;

    /**
     * Resets the current monotonic millis timer
     */
    void resetPulseTimer() noexcept;

    /**
     * Will insert a pulse result into the pulse ring buffer
     */
    void insertPulseResult(bool pulseResult) noexcept;

    /**
     * Compares the lasers actual state to its read state
     * @return
     */
    [[nodiscard]] bool checkLaserPulse() const noexcept;

    /**
     * Will complete a whole pulse cycle: verify, record, set new state
     */
    void doPulseCycle() noexcept;


    bool isInitialized = false;

    StateMachine& stateMachine;
    IRandom& i_random;
    ITime& i_time;
    LaserDiode laserDiode;
    LightEmittingDiode statusLed;
    LaserSensor laserSensor;

    time_t pulseTimer;
    bool isInitialPulse = true;
    // Stores pulse violations. each low bit represents one violation.
    uint32_t pulseRingBuffer = 0xFFFFFFFF;
    uint8_t pulseRingBufferPointer = 0;
    // Counts real pulses recorded since the ring buffer was last reset, capped at 32.
    // The violation count in pulseRingPuffer isn't meaningful until this reaches 32.
    uint8_t pulseSampleCount = 0;
};


#endif //LASERGATE_TESTS_GATEMODULE_H
