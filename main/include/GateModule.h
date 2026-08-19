#ifndef LASERGATE_TESTS_GATEMODULE_H
#define LASERGATE_TESTS_GATEMODULE_H

#include "LaserDiode.h"
#include "LightEmittingDiode.h"
#include "LaserSensor.h"
#include "StateMachine.h"

/**
 * Aggregates and drives a status led, a laser diode and a laser sensor into a module
 * that is able to detect objects interrupting the laser
 */
class GateModule {
public:
    GateModule(StateMachine& stateMachine, LaserDiode& laserDiode, LightEmittingDiode& statusLed, LaserSensor& laserSensor) noexcept;
    GateModule(const GateModule&) = delete;
    GateModule(GateModule&&) = delete;
    GateModule& operator=(const GateModule&) = delete;
    GateModule& operator=(GateModule&&) = delete;
    ~GateModule() noexcept;

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
     * @return Whether this moduls is ready for operation
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


    bool isInitialized = false;

    StateMachine& stateMachine;
    LaserDiode& laserDiode;
    LightEmittingDiode& statusLed;
    LaserSensor& laserSensor;
};


#endif //LASERGATE_TESTS_GATEMODULE_H
