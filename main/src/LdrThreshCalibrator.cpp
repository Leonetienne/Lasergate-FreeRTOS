#include "../include/LdrThreshCalibrator.h"
#include "GateModuleConfig.h"
#include <algorithm>

LdrThreshCalibrator::LdrThreshCalibrator(GateModule& module) noexcept:
    module {module}
{ }

void LdrThreshCalibrator::begin() noexcept {
    currentStatus = Status::RUNNING;
    reason.clear();
    phase = Phase::HOMING_LOWER;
    threshold = CALIB_LDR_THRESH_INITIAL_THRESH;
    lowerThreshold = 0;
    upperThreshold = 0;
    lastGoodThreshold = 0;

    if (!module.isInitialized) return;

    // laser diode on, calibration takes it from here
    if (!module.laserDiode.turnOn()) {
        currentStatus = Status::FAILED;
        reason = "LdrThreshCalibrator::begin: failed to turn on laser diode";
        return;
    }

    // Turn the status led off, if it is configured
    if (module.statusLed.isConfigured() && !module.statusLed.turnOff()) {
        currentStatus = Status::FAILED;
        reason = "LdrThreshCalibrator::begin: failed to turn off status LED";
        return;
    }

    module.laserSensor.setThreshold(threshold);
    module.resetPulseTimer();
    module.pulseHistory.reset();
}

void LdrThreshCalibrator::fixedUpdate() noexcept {
    if (currentStatus != Status::RUNNING) return;
    if (!module.isInitialized) return;

    // Run 32 pulses at the current threshold
    if (module.i_time.getMillis() - module.pulseTimer > CALIB_LDR_TRESH_PULSE_FREQ) {
        if (!module.doPulseCycle()) {
            return;
        }

        // Is this batch completed?
        if (module.pulseHistory.isSaturated()) {
            onBatchComplete();
            module.pulseHistory.reset();
        }
    }
}

LdrThreshCalibrator::Status LdrThreshCalibrator::status() const noexcept {
    return currentStatus;
}

const std::string& LdrThreshCalibrator::failureReason() const noexcept {
    return reason;
}

void LdrThreshCalibrator::onBatchComplete() noexcept {
    // Check whether the result set for this batch is acceptable
    if (module.isPulseBatchAcceptable().value_or(false)) {
        // Value is still good. Store this known-good thresh
        lastGoodThreshold = threshold;

        // Are we currently homing towards lower or upper boundary?
        switch (phase) {
            case Phase::HOMING_LOWER: {
                // If we have already reached the min threshold, hand off to homing upper.
                if (lastGoodThreshold == CALIB_LDR_TRESH_MIN_THRESH) {
                    advance();
                }
                // Else, reduce and check again
                else {
                    const uint16_t step = std::max<uint16_t>(CALIB_LDR_TRESH_MIN_STEP_SIZE, static_cast<uint16_t>(
                        static_cast<float>(lastGoodThreshold - CALIB_LDR_TRESH_MIN_THRESH) * CALIB_LDR_TRESH_STEP_FACTOR
                    ));
                    threshold = std::max<uint16_t>(lastGoodThreshold - step, CALIB_LDR_TRESH_MIN_THRESH);
                    module.laserSensor.setThreshold(threshold);
                }
                break;
            }
            case Phase::HOMING_UPPER: {
                // If we have already reached the max threshold, wrap up.
                if (lastGoodThreshold == CALIB_LDR_TRESH_MAX_THRESH) {
                    wrapUp();
                }
                // Else, increase and check again
                else {
                    const uint16_t step = std::max<uint16_t>(CALIB_LDR_TRESH_MIN_STEP_SIZE, static_cast<uint16_t>(
                        static_cast<float>(CALIB_LDR_TRESH_MAX_THRESH - lastGoodThreshold) * CALIB_LDR_TRESH_STEP_FACTOR
                    ));
                    threshold = std::min<uint16_t>(lastGoodThreshold + step, CALIB_LDR_TRESH_MAX_THRESH);
                    module.laserSensor.setThreshold(threshold);
                }
                break;
            }
        }
        return;
    }

    // Value is not good, but make sure it failed for the right reason: HOMING_LOWER should
    // fail with false positives, HOMING_UPPER with false negatives. Wrong shape means this
    // isn't actually the boundary, just noise.
    const bool expectedShape = (phase == Phase::HOMING_LOWER)
        ? module.pulseHistory.getFalsePositiveCount() > ALLOWED_MISREADS_PER_BATCH
        : module.pulseHistory.getFalseNegativeCount() > ALLOWED_MISREADS_PER_BATCH;

    if (!expectedShape) {
        currentStatus = Status::FAILED;
        reason = "LdrThreshCalibrator::onBatchComplete: batch failed with unexpected misread shape for phase " +
            std::string(phase == Phase::HOMING_LOWER ? "HOMING_LOWER" : "HOMING_UPPER") +
            ": false_positives=" + std::to_string(module.pulseHistory.getFalsePositiveCount()) +
            " false_negatives=" + std::to_string(module.pulseHistory.getFalseNegativeCount());
        return;
    }

    // Do we even have a good result?
    if (lastGoodThreshold == 0) {
        currentStatus = Status::FAILED;
        reason = "LdrThreshCalibrator::onBatchComplete: calibration failed! known_good_lower=" +
            std::to_string(lowerThreshold) + " known_good_higher=" + std::to_string(upperThreshold) +
            " last_known_good=" + std::to_string(lastGoodThreshold);
        return;
    }

    // Yes, we do. Treat last known good threshold value as the valid limit
    switch (phase) {
        // Still in first step, store intermittent result and hand off
        case Phase::HOMING_LOWER:
            advance();
            break;
        // Already in second step: this concludes the calibration
        case Phase::HOMING_UPPER:
            wrapUp();
            break;
    }
}

void LdrThreshCalibrator::advance() noexcept {
    lowerThreshold = lastGoodThreshold;
    lastGoodThreshold = 0;
    threshold = CALIB_LDR_THRESH_INITIAL_THRESH;
    module.laserSensor.setThreshold(threshold);
    phase = Phase::HOMING_UPPER;
}

void LdrThreshCalibrator::wrapUp() noexcept {
    upperThreshold = lastGoodThreshold;
    // Compute calibrated value
    const uint16_t calibratedThreshold = static_cast<uint16_t>(
        static_cast<float>(lowerThreshold) +
        ((static_cast<float>(upperThreshold) - static_cast<float>(lowerThreshold)) * CALIB_LDR_TRESH_TARGET_IN_RANGE)
    );
    module.laserSensor.setThreshold(calibratedThreshold);
    if (!module.settings.storeGateModuleLdrThreshold(module.settings_index, calibratedThreshold)) {
        currentStatus = Status::FAILED;
        reason = "LdrThreshCalibrator::wrapUp: failed to persist calibrated ldr threshold for module";
        return;
    }
    currentStatus = Status::CONCLUDED;
}
