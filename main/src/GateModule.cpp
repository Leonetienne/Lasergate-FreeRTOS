#include "../include/GateModule.h"
#include "GateModuleConfig.h"
#include "LaserPulseFreqCalibConfig.h"
#include "LdrThreshCalibConfig.h"
#include "compat/esp_log_macros.h"

static const char* LOG_TAG = "GateModule";

GateModule::GateModule(
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
) noexcept:
    stateMachine {stateMachine},
    settings {settings},
    settings_index {settingsIndex},
    i_random {i_random},
    i_time {i_time},
    laserDiode {pinRegister, i_gpio, laserPin},
    statusLed {pinRegister, i_gpio, statusLedPin},
    laserSensor {pinRegister, i_adcOneshot, ldrPin},
    pulseTimer (0)
{ }

GateModule::~GateModule() noexcept {
    if (isInitialized) {
        free();
    }
}

bool GateModule::isConfigured() const noexcept {
    return laserDiode.isConfigured() && laserSensor.isConfigured();
}

bool GateModule::initialize() noexcept {
    if (isInitialized) {
        return false;
    }

    bool success = true;

    // The status led is optional
    if (statusLed.isConfigured() && !statusLed.initialize()) {
        success = false;
    }
    if (!laserDiode.initialize()) {
        success = false;
    }

    const uint16_t ldrThreshold = settings.retrieveGateModuleLdrThreshold(settings_index).value_or(CALIB_LDR_THRESH_INITIAL_THRESH);
    if (!laserSensor.initialize(ldrThreshold)) {
        success = false;
    }

    laserPulseFrequency = settings.retrieveGateModuleLaserPulseFrequency(settings_index).value_or(CALIB_PULSE_FREQ_MAX_FREQ);

    resetPulseTimer();
    pulseHistory.reset();

    if (success) {
        isInitialized = true;
    }

    return success;
}

bool GateModule::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    bool success = true;

    if (!laserDiode.free()) {
        success = false;
    }
    if (statusLed.isReady() && !statusLed.free()) {
        success = false;
    }
    if (!laserSensor.free()) {
        success = false;
    }

    isInitialized = false;

    return success;
}

bool GateModule::isReady() const noexcept {
    return isInitialized;
}

void GateModule::fixedUpdate() noexcept {
    switch (stateMachine.getState()) {
        case STATE::FAULT:
            updateStateFault();
            break;
        case STATE::USER_ADJUSTING_BEAMS:
            updateStateUserAdjustingBeams();
            break;
        case STATE::CALIBRATION_LDR_THRESH:
            updateStateCalibrationLdrThresh();
            break;
        case STATE::CALIBRATION_MODULATION_FREQUENCY:
            updateStateCalibrationModulationFrequency();
            break;
        case STATE::OBSERVING:
            updateStateObserving();
            break;
        case STATE::DISARMED:
            updateStateDisarmed();

        break;
        default:
            // Shouldn't happen
            stateMachine.setState(STATE::FAULT, "GateModule::fixedUpdate: unhandled state");
            break;
    }
}

void GateModule::onStateChange() noexcept {
    switch (stateMachine.getState()) {
        case STATE::FAULT:
            onStateFault();
            break;
        case STATE::USER_ADJUSTING_BEAMS:
            onStateUserAdjustingBeams();
            break;
        case STATE::CALIBRATION_LDR_THRESH:
            onStateCalibrationLdrThresh();
            break;
        case STATE::CALIBRATION_MODULATION_FREQUENCY:
            onStateCalibrationModulationFrequency();
            break;
        case STATE::OBSERVING:
            onStateObserving();
            break;
        case STATE::DISARMED:
            onStateDisarmed();

            break;
        default: break; // Shouldn't happen
    }
}

uint16_t GateModule::getLdrThreshold() const noexcept {
    return laserSensor.getThreshold();
}

uint16_t GateModule::getPulseFrequency() const noexcept {
    return laserPulseFrequency;
}

void GateModule::onStateFault() noexcept {
    if (!isInitialized) return;
    laserDiode.turnOff();
    statusLed.turnOff();
}

/**
 * When the user is adjusting the beams, the user is turning screws by hand until the laser
 * is pointing directly on the LDR. The desired behavior to aid the user is:
 * - All lasers are constantly on
 * - All status leds (if configured) are on if the laser is hitting the LDR.
 */
void GateModule::onStateUserAdjustingBeams() noexcept {
    if (!isInitialized) return;

    // Turn the laser diode on (it stays that way)
    if (!laserDiode.turnOn()) {
        stateMachine.setState(STATE::FAULT, "GateModule::onStateUserAdjustingBeams: failed to turn on laser diode");
    }

    // Turn the status led off, if it is configured
    if (statusLed.isConfigured() && !statusLed.turnOff()) {
        stateMachine.setState(STATE::FAULT, "GateModule::onStateUserAdjustingBeams: failed to turn off status LED");
    }
}

void GateModule::updateStateUserAdjustingBeams() noexcept {
    if (!isInitialized) return;

    // Sync status led (if configured) state to laser hitting ldr
    if (statusLed.isConfigured()) {
        const auto reading = laserSensor.sensesLight();
        if (!reading.has_value()) {
            stateMachine.setState(STATE::FAULT, "GateModule::updateStateUserAdjustingBeams: failed to read laser sensor value");
            return;
        }
        statusLed.setPowerState(*reading);
    }
    resetPulseTimer();
    isInitialPulse = true;
}

/**
 * When the system is calibrating its LDR thresholds, it is establishing which raw sensor readings
 * are to be identified as "laser off" and which are "laser on".
 * - It should pulse the laser at a conservative frequency
 * - Slowly ramp up the pulse frequency until LDR readings differ from actual state
 * - To prevent patters from influencing the calibration, the pulse modulation is random
 */
void GateModule::onStateCalibrationLdrThresh() noexcept {
    if (!isInitialized) return;

    // Turn the laser diode off initially
    if (!laserDiode.turnOn()) {
        stateMachine.setState(STATE::FAULT, "GateModule::onStateCalibrationLdrThresh: failed to turn off laser diode");
    }

    // Turn the status led off, if it is configured
    if (statusLed.isConfigured() && !statusLed.turnOff()) {
        stateMachine.setState(STATE::FAULT, "GateModule::onStateCalibrationLdrThresh: failed to turn off status LED");
    }

    calib_ldr_state = CALIB_LDR_STATE::HOMING_LOWER;

    resetPulseTimer();
    pulseHistory.reset();
}

/**
 * Calibrating LDR threshold:
 * - Home lower limit (just before false positives take over: ambient light becomes too bright)
 * - Home upper limit (just before false negatives take over: laser is not bright enough)
 * - Calibrated value is the somewhere on the established good value range
*/
void GateModule::updateStateCalibrationLdrThresh() noexcept {
    if (!isInitialized) return;

    // Run 32 pulses at initial threshold
    if (i_time.getMillis() - pulseTimer > CALIB_LDR_TRESH_PULSE_FREQ) {
        doPulseCycle();

        // Is this batch completed?
        if (pulseHistory.getSampleCount() == 32) {
            // Check whether the result set for this batch is acceptable
            if (isPulseBatchAcceptable().value_or(false)) {
                // Value is still good.

                // Store this known-good thresh
                calib_ldr_last_good_threshold = laserSensor.getThreshold();

                // Are we currently homing towards lower or upper boundary?
                switch (calib_ldr_state) {
                    case CALIB_LDR_STATE::HOMING_LOWER: {
                        // Reduce and check again
                        const int step = std::max<int>(CALIB_LDR_TRESH_MIN_STEP_SIZE, static_cast<int>(
                            static_cast<float>(calib_ldr_last_good_threshold - CALIB_LDR_TRESH_MIN_THRESH) * CALIB_LDR_TRESH_STEP_FACTOR
                        ));
                        laserSensor.setThreshold(std::max<int>(calib_ldr_last_good_threshold - step, CALIB_LDR_TRESH_MIN_THRESH));
                        break;
                    }
                    case CALIB_LDR_STATE::HOMING_UPPER: {
                        // Increase and check again
                        const int step = std::max<int>(CALIB_LDR_TRESH_MIN_STEP_SIZE, static_cast<int>(
                            static_cast<float>(CALIB_LDR_TRESH_MAX_THRESH - calib_ldr_last_good_threshold) * CALIB_LDR_TRESH_STEP_FACTOR
                        ));
                        laserSensor.setThreshold(std::min<int>(calib_ldr_last_good_threshold + step, CALIB_LDR_TRESH_MAX_THRESH));
                        break;
                    }
                    default:
                        stateMachine.setState(STATE::FAULT, "GateModule::updateStateCalibrationLdrThresh: ended up in invalid state (CALIB_LDR_STATE::NONE) during incrementing/decrementing limit");
                        break;
                }
            }
            // Value is not good. Treat last known good threshold value as valid limit
            else {
                // Do we even have a good result?
                if (calib_ldr_last_good_threshold == 0) {
                    stateMachine.setState(STATE::FAULT, "GateModule::updateStateCalibrationLdrThresh: calibration failed! known_good_lower=" +
                        std::to_string(calib_ldr_lower_threshold) + " known_good_higher=" + std::to_string(calib_ldr_upper_threshold) +
                        " last_know_good=" + std::to_string(calib_ldr_last_good_threshold));
                    return;
                }

                // Yes, we do.
                switch (calib_ldr_state) {
                    // Still in first step, store intermittent result and reset
                    case CALIB_LDR_STATE::HOMING_LOWER: {
                        calib_ldr_lower_threshold = calib_ldr_last_good_threshold;
                        calib_ldr_last_good_threshold = 0;
                        laserSensor.setThreshold(CALIB_LDR_THRESH_INITIAL_THRESH);
                        calib_ldr_state = CALIB_LDR_STATE::HOMING_UPPER;
                        break;
                    }
                    // Already in second step: this concludes the calibration
                    case CALIB_LDR_STATE::HOMING_UPPER: {
                        calib_ldr_upper_threshold = calib_ldr_last_good_threshold;
                        calib_ldr_last_good_threshold = 0;
                        // Compute calibrated value
                        const int calib_ldr_calibrated_thresh = static_cast<int>(
                            static_cast<float>(calib_ldr_lower_threshold) +
                            ((static_cast<float>(calib_ldr_upper_threshold) - static_cast<float>(calib_ldr_lower_threshold)) * CALIB_LDR_TRESH_TARGET_IN_RANGE)
                        );
                        laserSensor.setThreshold(calib_ldr_calibrated_thresh);
                        if (!settings.storeGateModuleLdrThreshold(settings_index, static_cast<uint16_t>(calib_ldr_calibrated_thresh))) {
                            ESP_LOGW(LOG_TAG, "failed to persist calibrated ldr threshold for module %zu", settings_index);
                        }
                        calib_ldr_state = CALIB_LDR_STATE::NONE;
                        stateMachine.setState(STATE::DISARMED);
                        break;
                    }
                    default:
                        stateMachine.setState(STATE::FAULT, "GateModule::updateStateCalibrationLdrThresh: ended up in invalid state (CALIB_LDR_STATE::NONE) during storing result");
                        break;
                }
            }


            // Reset for new batch
            pulseHistory.reset();
        }
    }
}

void GateModule::onStateCalibrationModulationFrequency() noexcept {
    if (!isInitialized) return;
}

void GateModule::onStateObserving() noexcept {
    if (!isInitialized) return;
}

void GateModule::onStateAlarm() noexcept {
    if (!isInitialized) return;
}

void GateModule::onStateDisarmed() noexcept {
    if (!isInitialized) return;
}

void GateModule::updateStateFault() noexcept {
    if (!isInitialized) return;
}

void GateModule::updateStateCalibrationModulationFrequency() noexcept {
}

void GateModule::updateStateObserving() noexcept {
    if (!isInitialized) return;
}

void GateModule::updateStateAlarm() noexcept {
    if (!isInitialized) return;
}

void GateModule::updateStateDisarmed() noexcept {
    if (!isInitialized) return;
}

void GateModule::applyPulseTarget() noexcept {
    if (!isInitialized) return;
    laserDiode.setPowerState(i_random.getNextBit());
}

void GateModule::resetPulseTimer() noexcept {
    if (!isInitialized) return;
    pulseTimer = i_time.getMillis();
}

std::optional<std::pair<bool, bool>> GateModule::readPulseState() const noexcept {
    const auto sensorReading = laserSensor.sensesLight();
    if (!sensorReading.has_value()) {
        stateMachine.setState(STATE::FAULT, "GateModule::readPulseState: failed to read laser sensor");
        return std::nullopt;
    }
    const auto laserState = laserDiode.getPowerState();
    if (!laserState.has_value()) {
        stateMachine.setState(STATE::FAULT, "GateModule::readPulseState: failed to read laser status");
        return std::nullopt;
    }
    return std::make_pair(*sensorReading, *laserState);
}

void GateModule::doPulseCycle() noexcept {
    // The LDR needs some time to pull up after initial light hit.
    // This is why the pulse frequency is critical.
    // This implies that we must pulse like this:
    // SET_STATE -> DELAY -> VERIFY -> ...
    // Since the verify method is delay-guarded, the verification must happen before setting state

    if (const auto pulseState = readPulseState(); pulseState.has_value()) {
        pulseHistory.insertResult(pulseState->first, pulseState->second);
    }
    applyPulseTarget();
    resetPulseTimer();
}

std::optional<bool> GateModule::isPulseBatchAcceptable() const noexcept {
    if (!isInitialized) return std::nullopt;
    if (pulseHistory.getSampleCount() != 32) return std::nullopt;

    return pulseHistory.getFailureCount() <= ALLOWED_MISREADS_PER_BATCH;
}
