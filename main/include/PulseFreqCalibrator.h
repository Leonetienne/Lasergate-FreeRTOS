#ifndef LASERGATE_V2_PULSEFREQCALIBRATOR_H
#define LASERGATE_V2_PULSEFREQCALIBRATOR_H

#include "GateModule.h"
#include "LaserPulseFreqCalibConfig.h"
#include <string>

/**
 * Homes a GateModule's laser pulse frequency: pulses at a known frequency, lowers it while
 * readings stay clean, and keeps the last good frequency as the calibrated value.
 */
class PulseFreqCalibrator {
public:
    explicit PulseFreqCalibrator(GateModule& module) noexcept;

    /**
     * (Re-)starts calibration, status included.
     */
    void begin() noexcept;

    /**
     * Ticks pulse sampling + decision logic. No-op once concluded or failed.
     */
    void fixedUpdate() noexcept;

    enum class Status { RUNNING, CONCLUDED, FAILED };

    [[nodiscard]] Status status() const noexcept;

    /**
     * Only valid once status() == FAILED.
     */
    [[nodiscard]] const std::string& failureReason() const noexcept;

private:
    /**
     * Called once a batch of 32 is in. Keep lowering the frequency, or wrap up.
     */
    void onBatchComplete() noexcept;

    /**
     * Applies + persists the last good frequency, done (or FAILED if we never found one, or
     * the store fails).
     */
    void wrapUp() noexcept;

    GateModule& module;
    Status currentStatus = Status::RUNNING;
    std::string reason;
    uint16_t frequency = CALIB_PULSE_FREQ_MAX_FREQ;
    uint16_t lastGoodFrequency = 0;
};

#endif //LASERGATE_V2_PULSEFREQCALIBRATOR_H
