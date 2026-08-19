#include "../include/GateModule.h"

constexpr int INITIAL_LDR_THRESH = 3000;

GateModule::GateModule(
    StateMachine& stateMachine,
    GpioPinRegister& pinRegister,
    IGpio& gpio,
    IAdcOneshot& adcOneshot,
    gpio_num_t laserPin,
    gpio_num_t statusLedPin,
    gpio_num_t ldrPin
) noexcept:
    stateMachine {stateMachine},
    laserDiode {pinRegister, gpio, laserPin},
    statusLed {pinRegister, gpio, statusLedPin},
    laserSensor {pinRegister, adcOneshot, ldrPin}
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
    if (!laserDiode.turnOn()) {
        stateMachine.setState(STATE::FAULT, "GateModule::onStateUserAdjustingBeams: failed to turn on laser diode");
    }
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
}

void GateModule::onStateCalibrationLdrThresh() noexcept {
    if (!isInitialized) return;
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

void GateModule::updateStateCalibrationLdrThresh() noexcept {
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
