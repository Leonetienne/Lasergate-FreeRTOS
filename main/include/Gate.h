#ifndef LASERGATE_TESTS_GATE_H
#define LASERGATE_TESTS_GATE_H

#include "GateModule.h"
#include "GpioPinRegister.h"
#include "LdrThreshCalibrator.h"
#include "PulseFreqCalibrator.h"
#include "SettingsManager.h"
#include "StateMachine.h"
#include "hal/IAdcOneshot.h"
#include "hal/IGpio.h"
#include "hal/IRandom.h"
#include "hal/ITime.h"
#include <array>

/**
 * A gate aggregates four gate modules into a single gate. Reads each module's
 * gpio pins from settings at construction time.
 * The gate raises alarm state if any of the following conditions is met:
 * - Less than or equal n; n>=1, gatemodules are interrupted for more than x ms
 * - More than n gatemodules are interrupted for any amount of time
 */
class Gate {
public:
    static constexpr std::size_t MODULE_COUNT = 4;

    Gate(
        StateMachine& stateMachine,
        SettingsManager& settings,
        GpioPinRegister& gpioPinRegister,
        IGpio& i_gpio,
        IAdcOneshot& i_adcOneshot,
        IRandom& i_random,
        ITime& i_time
    ) noexcept;
    Gate(const Gate&) = delete;
    Gate(Gate&&) = delete;
    Gate& operator=(const Gate&) = delete;
    Gate& operator=(Gate&&) = delete;
    ~Gate() noexcept;

    /**
      * Will initialize the gate
      * @return Success state
      */
    bool initialize() noexcept;

    /**
     * Will free all resources acquired by this object
     * @return Success state
     */
    bool free() noexcept;

    /**
     * @return Whether this gate is ready for operation
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
    void resetLenientAlarmState() noexcept;
    void startLenientAlarmState() noexcept;

    /**
     * Will set longestBatchTime to the duration in ms the slowest pulsing
     * module requires to complete a batch.
     */
    void recalculateLongestBatchTime() noexcept;

    bool isInitialized = false;

    StateMachine& stateMachine;
    // keep the calibrators declared after modules. they hold references into it, order matters
    std::array<GateModule, MODULE_COUNT> modules;
    std::array<LdrThreshCalibrator, MODULE_COUNT> ldrCalibrators;
    std::array<PulseFreqCalibrator, MODULE_COUNT> freqCalibrators;

    ITime& i_time;

    time_t lenientAlarmTimeout;
    bool lenientAlarmActive = false;
    uint16_t longestBatchTime = 0;
};

#endif //LASERGATE_TESTS_GATE_H
