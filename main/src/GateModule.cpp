#include "GateModule.h"
#include "GateModuleConfig.h"
#include "LaserPulseFreqCalibConfig.h"
#include "LdrThreshCalibConfig.h"
#include "compat/esp_log_macros.h"

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
        case STATE::OBSERVING:
            updateStateObserving();
            break;
        case STATE::ALARM:
            updateStateAlarm();
            break;
        case STATE::DIAGNOSTIC_SIGNAL_TEST_RUN:
            updateStateDiagnosticSignalTestRun();
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
        case STATE::OBSERVING:
            onStateObserving();
            break;
        case STATE::ALARM:
            onStateAlarm();
            break;
        case STATE::DIAGNOSTIC_SIGNAL_TEST_RUN:
            onStateDiagnosticSignalTestRun();
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
        return;
    }

    // Turn the status led off, if it is configured
    if (statusLed.isConfigured() && !statusLed.turnOff()) {
        ESP_LOGW(LOG_TAG, "GateModule::onStateUserAdjustingBeams: failed to set status led state during onStateUserAdjustingBeams");
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

        if (!statusLed.setPowerState(*reading)) {
            ESP_LOGW(LOG_TAG, "GateModule::updateStateUserAdjustingBeams: failed to set status led state during updateStateUserAdjustingBeams");
        }
    }
    resetPulseTimer();
}

/**
 * Beginning observing should turn lasers off and status leds on initially.
 * Also reset pulse helpers.
 */
void GateModule::onStateObserving() noexcept {
    if (!isInitialized) return;

    // Turn the laser diode off initially
    if (!laserDiode.turnOff()) {
        stateMachine.setState(STATE::FAULT, "GateModule::onStateObserving: failed to turn off laser diode");
        return;
    }

    // Turn the status led on initially; after the ring puffer saturates, it mirrors the last pulse reading value, misread being HIGH
    if (statusLed.isConfigured() && !statusLed.turnOn()) {
        ESP_LOGW(LOG_TAG, "GateModule::onStateObserving: failed to turn on status LED during onStateObserving");
    }

    pulseHistory.reset();
    resetPulseTimer();
}

/**
 * Pulse the laser and record to pulseHistory.
 * Turn status LED off after ring puffer is saturated and blink status LED on misreads.
 * Alarm state can't be raised by a single module as the decision to raise an alarm depends
 * on multiple modules simultaneously.
 * Gate is supposed to check isPulseBatchAcceptable() and raise alarm state themselves.
 */
void GateModule::updateStateObserving() noexcept {
    if (!isInitialized) return;

    if (i_time.getMillis() - pulseTimer > laserPulseFrequency) {
        if (!doPulseCycle()) {
            return;
        }

        // Don't do anything until the ring puffer is saturated (reaching a meaningful state)
        if (pulseHistory.isSaturated()) {
            // Status led should mirror the last pulse reading value, misread being HIGH
            if (
                statusLed.isConfigured() &&
                !statusLed.setPowerState(!pulseHistory.at(0))
            ) {
                ESP_LOGW(LOG_TAG, "GateModule::updateStateObserving: failed to set status led state during updateStateObserving");
            }
        }
    }
}

/**
 * During alarm, blink the status led on/off, being on initially
 */
void GateModule::onStateAlarm() noexcept {
    if (!isInitialized) return;

    if (!laserDiode.turnOff()) {
        stateMachine.setState(STATE::FAULT, "GateModule::onStateAlarm: failed to turn off laser diode");
    }

    // Turn the status led on, if it is configured
    if (statusLed.isConfigured() && !statusLed.turnOn()) {
        ESP_LOGW(LOG_TAG, "GateModule::onStateAlarm: failed to set status led state during onStateAlarm");
    }

    // To simplify, we'll just use the pulseTimer to time the status led
    resetPulseTimer();
    pulseHistory.reset();
}

void GateModule::updateStateAlarm() noexcept {
    if (!isInitialized) return;

    if (statusLed.isConfigured()) {
        // To simplify, we'll just use the pulseTimer to time the status led
        if (i_time.getMillis() - pulseTimer > ALARM_STATE_STATUS_LED_BLINK_INTERVAL) {
            resetPulseTimer();

            if (!statusLed.toggle()) {
                ESP_LOGW(LOG_TAG, "GateModule::updateStateAlarm: failed to toggle status led state during updateStateAlarm");
            }
        }
    }
}

/**
 * When initiating a test run to evaluate channel noise, first reset the misread counter.
 * turn laser and status led off.
 */
void GateModule::onStateDiagnosticSignalTestRun() noexcept {
    if (!isInitialized) return;

    if (!laserDiode.turnOff()) {
        stateMachine.setState(STATE::FAULT, "GateModule::onStateDiagnosticSignalTestRun: failed to turn off laser diode");
    }

    // Turn the status led off, if it is configured
    if (statusLed.isConfigured() && !statusLed.turnOff()) {
        ESP_LOGW(LOG_TAG, "GateModule::onStateDiagnosticSignalTestRun: failed to turn status led off during onStateAlarm");
    }

    diagnosticSignalTestRunNumMisreads = 0;
    diagnosticSignalTestRunNumBatchesRun = 0;
}

/**
 * Pulse the laser and record to pulseHistory.
 * After each batch increment failure count.
 * After reaching the final batch, fall back to DISARMED.
 */
void GateModule::updateStateDiagnosticSignalTestRun() noexcept {
    if (!isInitialized) return;

    // Run a batch worth of pulses at the current frequency
    if (i_time.getMillis() - pulseTimer > laserPulseFrequency) {
        if (!doPulseCycle()) {
            return;
        }

        // Is the batch finished?
        if (pulseHistory.isSaturated()) {
            // Store misreads
            diagnosticSignalTestRunNumMisreads += pulseHistory.getFailureCount();

            // Reset the current ring buffer
            pulseHistory.reset();

            // Have we finished all batches?
            if (diagnosticSignalTestRunNumBatchesRun++ >= DIAGNOSTIC_SIGNAL_TEST_NUM_BATCHES) {
                stateMachine.setState(STATE::DISARMED);
            }
        }
    }
}

/**
 *  * During fault, blink the status led on/off, being on initially
 */
void GateModule::onStateFault() noexcept {
    if (!isInitialized) return;

    if (!laserDiode.turnOff()) {
        // Just a warning, since we're already in fault mode
        ESP_LOGW(LOG_TAG, "GateModule::onStateFault: failed to turn off laser diode during onStateFault");
    }

    // Turn the status led on, if it is configured
    if (statusLed.isConfigured() && !statusLed.turnOn()) {
        ESP_LOGW(LOG_TAG, "GateModule::onStateFault: failed to turn status led off during onStateFault");
    }

    // To simplify, we'll just use the pulseTimer to time the status led
    resetPulseTimer();
    pulseHistory.reset();
}


void GateModule::updateStateFault() noexcept {
    if (!isInitialized) return;

    if (statusLed.isConfigured()) {
        // To simplify, we'll just use the pulseTimer to time the status led
        if (i_time.getMillis() - pulseTimer > FAULT_STATE_STATUS_LED_BLINK_INTERVAL) {
            resetPulseTimer();

            if (!statusLed.toggle()) {
                ESP_LOGW(LOG_TAG, "GateModule::updateStateFault: failed to toggle status led state during updateStateFault");
            }
        }
    }
}

/**
 * When disarming, turn laser off and statu led on
 */
void GateModule::onStateDisarmed() noexcept {
    if (!isInitialized) return;

    if (!laserDiode.turnOff()) {

        stateMachine.setState(STATE::FAULT, "GateModule::onStateDisarmed: failed to turn off laser diode");
    }

    // Turn the status led on, if it is configured
    if (statusLed.isConfigured() && !statusLed.turnOn()) {
        ESP_LOGW(LOG_TAG, "GateModule::onStateDisarmed: failed to set status led state during onStateDisarmed");
    }
}

/**
 * No need to do anything during disarmed state
 */
void GateModule::updateStateDisarmed() noexcept {
    if (!isInitialized) return;
}

bool GateModule::applyPulseTarget() noexcept {
    if (!isInitialized) return false;
    if (!laserDiode.setPowerState(i_random.getNextBit())) {
        stateMachine.setState(STATE::FAULT, "GateModule::applyPulseTarget: failed to set laser power state");
        return false;
    }
    return true;
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

bool GateModule::doPulseCycle() noexcept {
    // The LDR needs some time to pull up after initial light hit.
    // This is why the pulse frequency is critical.
    // This implies that we must pulse like this:
    // SET_STATE -> DELAY -> VERIFY -> ...
    // Since the verify method is delay-guarded, the verification must happen before setting state

    const auto pulseState = readPulseState();
    if (!pulseState.has_value()) return false;

    pulseHistory.insertResult(pulseState->first, pulseState->second);
    if (!applyPulseTarget()) {
        return false;
    }
    resetPulseTimer();

    return true;
}

std::optional<bool> GateModule::isPulseBatchAcceptable() const noexcept {
    if (!isInitialized) return std::nullopt;
    if (!pulseHistory.isSaturated()) return std::nullopt;

    return pulseHistory.getFailureCount() <= ALLOWED_MISREADS_PER_BATCH;
}

std::optional<uint16_t> GateModule::getBatchTime() const noexcept {
    if (!isInitialized || !isConfigured() || laserPulseFrequency == 0) {
        return std::nullopt;
    }

    return PulseRingBuffer::getBufferSize() * laserPulseFrequency;
}

std::optional<uint16_t> GateModule::getLastDiagnosticRunSignalError() const noexcept {
    if (!isInitialized) return std::nullopt;
    return diagnosticSignalTestRunNumMisreads;
}
