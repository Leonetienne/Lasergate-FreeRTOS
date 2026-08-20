#include "../include/GateModule.h"

constexpr int64_t INITIAL_LDR_THRESH = 3000;

constexpr int64_t CALIB_LDR_TRESH_PULSE_FREQ = 500; // ms

GateModule::GateModule(
    StateMachine& stateMachine,
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
    if (!laserSensor.initialize(INITIAL_LDR_THRESH)) {
        success = false;
    }

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

    resetPulseTimer();
    pulseHistory.reset();
}

void GateModule::updateStateCalibrationLdrThresh() noexcept {
    if (!isInitialized) return;

    // The LDR needs some time to pull up after initial light hit.
    // This is why the pulse frequency is critical.
    // This implies that we must pulse like this:
    // SET_STATE -> DELAY -> VERIFY -> ...
    // Since the verify method is delay-guarded, the verification must happen before setting state


    if (i_time.getMillis() - pulseTimer > CALIB_LDR_TRESH_PULSE_FREQ) {
        doPulseCycle();

        // Is this batch completed?
        if (pulseHistory.getSampleCount() == 32) {
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
    if (const auto pulseState = readPulseState(); pulseState.has_value()) {
        pulseHistory.insertResult(pulseState->first, pulseState->second);
    }
    applyPulseTarget();
    resetPulseTimer();
}
