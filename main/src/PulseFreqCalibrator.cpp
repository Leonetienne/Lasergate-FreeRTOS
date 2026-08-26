#include "../include/PulseFreqCalibrator.h"
#include <algorithm>

PulseFreqCalibrator::PulseFreqCalibrator(GateModule& module) noexcept:
    module {module}
{ }

void PulseFreqCalibrator::begin() noexcept {
    currentStatus = Status::RUNNING;
    reason.clear();
    frequency = CALIB_PULSE_FREQ_MAX_FREQ;
    lastGoodFrequency = 0;

    if (!module.isInitialized) return;

    module.laserPulseFrequency = frequency;

    // laser diode on, calibration takes it from here
    if (!module.laserDiode.turnOn()) {
        currentStatus = Status::FAILED;
        reason = "PulseFreqCalibrator::begin: failed to turn on laser diode";
        return;
    }

    // Turn the status led off, if it is configured
    if (module.statusLed.isConfigured() && !module.statusLed.turnOff()) {
        currentStatus = Status::FAILED;
        reason = "PulseFreqCalibrator::begin: failed to turn off status LED";
        return;
    }

    module.resetPulseTimer();
    module.pulseHistory.reset();
}

void PulseFreqCalibrator::fixedUpdate() noexcept {
    if (currentStatus != Status::RUNNING) return;
    if (!module.isInitialized) return;

    // Run 32 pulses at the current frequency
    if (module.i_time.getMillis() - module.pulseTimer > frequency) {
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

PulseFreqCalibrator::Status PulseFreqCalibrator::status() const noexcept {
    return currentStatus;
}

const std::string& PulseFreqCalibrator::failureReason() const noexcept {
    return reason;
}

void PulseFreqCalibrator::onBatchComplete() noexcept {
    if (!module.isPulseBatchAcceptable().value_or(false)) {
        wrapUp();
        return;
    }

    // Value is still good. Store known-good freq, reduce and try again
    lastGoodFrequency = frequency;

    // If we have already reached the min freq, wrap up.
    if (lastGoodFrequency == CALIB_PULSE_FREQ_MIN_FREQ) {
        wrapUp();
        return;
    }

    // Else, reduce and try again
    const uint16_t step = std::max<uint16_t>(CALIB_PULSE_FREQ_MIN_STEP_SIZE, static_cast<uint16_t>(
        static_cast<float>(lastGoodFrequency - CALIB_PULSE_FREQ_MIN_FREQ) * CALIB_PULSE_FREQ_STEP_FACTOR
    ));
    frequency = std::max<uint16_t>(lastGoodFrequency - step, CALIB_PULSE_FREQ_MIN_FREQ);
    module.laserPulseFrequency = frequency;
}

void PulseFreqCalibrator::wrapUp() noexcept {
    // Treat the last good frequency as the final result. Do we even have a good result?
    if (lastGoodFrequency == 0) {
        currentStatus = Status::FAILED;
        reason = "PulseFreqCalibrator::wrapUp: calibration failed: reached unacceptable pulse batch before reaching any valid frequency";
        return;
    }
    frequency = lastGoodFrequency;
    module.laserPulseFrequency = lastGoodFrequency;
    if (!module.settings.storeGateModuleLaserPulseFrequency(module.settings_index, lastGoodFrequency)) {
        currentStatus = Status::FAILED;
        reason = "PulseFreqCalibrator::wrapUp: failed to persist calibrated laser pulse frequency for module";
        return;
    }
    currentStatus = Status::CONCLUDED;
}
