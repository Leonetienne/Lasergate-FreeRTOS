#include "../include/Diode.h"

Diode::Diode(GpioDigitalWritePin &gpioPin) noexcept :
    gpioPin { gpioPin }
{}

Diode::~Diode() noexcept {
    if (isInitialized) {
        free();
    }
}

bool Diode::initialize() noexcept {
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

bool Diode::free() noexcept {
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

bool Diode::isReady() const noexcept {
    return isInitialized;
}

bool Diode::turnOn() const noexcept {
    if (!isInitialized) {
        return false;
    }

    return gpioPin.setState(PIN_STATE_DIGITAL::HIGH);
}

bool Diode::turnOff() const noexcept {
    if (!isInitialized) {
        return false;
    }

    return gpioPin.setState(PIN_STATE_DIGITAL::LOW);
}

bool Diode::setPowerState(bool desiredPowerState) noexcept {
    if (!isInitialized) {
        return false;
    }

    if (desiredPowerState) {
        return turnOn();
    }
    return turnOff();
}

std::expected<bool, bool> Diode::getPowerState() const noexcept {
    if (!isInitialized) {
        return std::unexpected(false);
    }
    return powerState;
}
