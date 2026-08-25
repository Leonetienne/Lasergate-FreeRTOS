#ifndef LASERGATE_V2_LDRTHRESHCALIBRATOR_H
#define LASERGATE_V2_LDRTHRESHCALIBRATOR_H

#include "GateModule.h"
#include "LdrThreshCalibConfig.h"
#include <string>

/**
 * Homes a GateModule's LDR threshold: finds a lower bound (just before ambient light starts
 * false-positiving) and an upper bound (just before the laser's own brightness false-negatives),
 * then settles on a value somewhere in between.
 */
class LdrThreshCalibrator {
public:
    explicit LdrThreshCalibrator(GateModule& module) noexcept;

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
     * Called once a batch of 32 is in. Keep homing, hand off lower->upper, wrap up, or fail.
     */
    void onBatchComplete() noexcept;

    /**
     * Lower bound found. stash it, reset to the starting threshold, move to HOMING_UPPER.
     */
    void advance() noexcept;

    /**
     * Both bounds found: pick a value, apply + persist it, done (or FAILED if the store fails).
     */
    void wrapUp() noexcept;

    GateModule& module;
    Status currentStatus = Status::RUNNING;
    std::string reason;
    uint16_t threshold = CALIB_LDR_THRESH_INITIAL_THRESH;
    uint16_t lowerThreshold = 0;
    uint16_t upperThreshold = 0;
    uint16_t lastGoodThreshold = 0;
    enum class Phase { HOMING_LOWER, HOMING_UPPER } phase = Phase::HOMING_LOWER;
};

#endif //LASERGATE_V2_LDRTHRESHCALIBRATOR_H
