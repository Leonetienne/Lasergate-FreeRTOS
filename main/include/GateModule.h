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

class LdrThreshCalibrator;
class PulseFreqCalibrator;

/**
 * Aggregates and drives a status led, a laser diode and a laser sensor into a module
 * that is able to detect objects interrupting the laser
 */
class GateModule {
    // calibrators need direct access to the hardware/pulse internals below.
    // they must never mutate the state machine.
    friend class LdrThreshCalibrator;
    friend class PulseFreqCalibrator;

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

    /**
     * @return Whether pulseHistory holds a batch that's good enough to be considered healthy (Num of misreads within tolerance)
     */
    [[nodiscard]] std::optional<bool> isPulseBatchAcceptable() const noexcept;

    /**
     * @return The amount in ms the module takes to completely a pulse batch, or std::nullopt if the gate is not ready or does not have a pulse frequency assigned yet
     */
    [[nodiscard]] std::optional<uint16_t> getBatchTime() const noexcept;

    /**
     * @return How many misreads happened during the last DIAGNOSTIC_SIGNAL_TEST_RUN run
     * (0 if no such run occurred), std::nullopt if uninitialized or if the run is not yet finished
     */
    [[nodiscard]] std::optional<uint16_t> getLastDiagnosticRunSignalError() const noexcept;

    /**
     * @return Whether this module finished an ongoing DIAGNOSTIC_SIGNAL_TEST_RUN.
     * std::nullopt if uninitialized
     */
    [[nodiscard]] std::optional<bool> isDiagnosticSignalTestRunFinished() const noexcept;

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
     * Gets called once after state engine switches to OBSERVING
     */
    void onStateObserving() noexcept;

    /**
     * Gets called once after state engine switches to ALARM
     */
    void onStateAlarm() noexcept;

    /**
     * Gets called once after state engine switches to DIAGNOSTIC_SIGNAL_TEST_RUN
     */
    void onStateDiagnosticSignalTestRun() noexcept;

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
     * Gets called by update during state OBSERVING
     */
    void updateStateObserving() noexcept;

    /**
     * Gets called by update during state ALARM
     */
    void updateStateAlarm() noexcept;

    /**
     * Gets called by update during state DIAGNOSTIC_SIGNAL_TEST_RUN
     */
    void updateStateDiagnosticSignalTestRun() noexcept;

    /**
      * Gets called by update during state DISARMED
      */
    void updateStateDisarmed() noexcept;

    /**
     * Will set decide a new pulse laser state and apply it to the laser diode
     * @return success state (false as no-opt on call on uninitialized object; false with FAULT raised when acting upon laser power state fails)
     */
    bool applyPulseTarget() noexcept;

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
     * @return success state (if false, FAULT state was already raised)
     */
    bool doPulseCycle() noexcept;

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
    uint16_t diagnosticSignalTestRunNumMisreads = 0;
    uint8_t diagnosticSignalTestRunNumBatchesRun = 0;
};


#endif //LASERGATE_TESTS_GATEMODULE_H
