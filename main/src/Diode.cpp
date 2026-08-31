#include "../include/Diode.h"
#include "compat/esp_log_macros.h"

static const char* LOG_TAG = "Diode";

Diode::Diode(GpioPinRegister& pinRegister, IGpio& i_gpio, gpio_num_t pinNum) noexcept :
    gpioPin { pinRegister, i_gpio, pinNum }
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

bool Diode::isConfigured() const noexcept {
    return gpioPin.getGpioNum() != GPIO_NUM_NC;
}

bool Diode::turnOn() noexcept {
    if (!isInitialized) {
        return false;
    }

    return gpioPin.setState(PIN_STATE_DIGITAL::HIGH);
}

bool Diode::turnOff() noexcept {
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
    return gpioPin.getState() == PIN_STATE_DIGITAL::HIGH;
}

bool Diode::toggle() noexcept {
    if (!isInitialized) {
        return false;
    }

    const auto currentState = getPowerState();
    if (!currentState.has_value()) {
        ESP_LOGW(LOG_TAG, "toggle: failed to read current power state");
        return false;
    }

    if (!setPowerState(!*currentState)) {
        ESP_LOGW(LOG_TAG, "toggle: failed to set new power state");
        return false;
    }

    return true;
}
