#include "../include/LaserDiode.h"

LaserDiode::LaserDiode(GpioDigitalWritePin &gpioPin) noexcept :
    gpioPin { gpioPin }
{}

LaserDiode::~LaserDiode() noexcept {
    if (isInitialized) {
        free();
    }
}

bool LaserDiode::initialize() noexcept {
    if (isInitialized) {
        return false;
    }

    bool success = true;

    if (!gpioPin.initialize()) {
        success = false;
    }

    if (success) {
        isInitialized = true;
    }

    return success;
}

bool LaserDiode::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    bool success = true;

    if (!gpioPin.free()) {
        success = false;
    }

    isInitialized = false;

    return success;
}

bool LaserDiode::isReady() const noexcept {
    return isInitialized;
}

bool LaserDiode::turnOn() const noexcept {
    if (!isInitialized) {
        return false;
    }

    return gpioPin.setState(PIN_STATE_DIGITAL::HIGH);
}

bool LaserDiode::turnOff() const noexcept {
    if (!isInitialized) {
        return false;
    }

    return gpioPin.setState(PIN_STATE_DIGITAL::LOW);
}

bool LaserDiode::setPowerState(bool desiredPowerState) noexcept {
    if (!isInitialized) {
        return false;
    }

    if (desiredPowerState) {
        return turnOn();
    }
    return turnOff();
}

std::expected<bool, bool> LaserDiode::getPowerState() const noexcept {
    if (!isInitialized) {
        return std::unexpected(false);
    }
    return powerState;
}
