#include "LaserSensor.h"

LaserSensor::LaserSensor(GpioPinRegister& pinRegister, IAdcOneshot& i_adcOneshot, gpio_num_t pinNum) noexcept:
    ldrPin {pinRegister, i_adcOneshot, pinNum}
{ }

LaserSensor::~LaserSensor() noexcept {
    if (isInitialized) {
        free();
    }
}

bool LaserSensor::initialize(int desiredThreshold) noexcept {
    if (isInitialized) {
        return false;
    }

    bool success = true;

    if (ldrPin.initialize() != ESP_OK) {
        success = false;
    }

    threshold = desiredThreshold;

    if (success) {
        isInitialized = true;
    }

    return success;
}

bool LaserSensor::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    bool success = true;

    if (!ldrPin.free()) {
        success = false;
    }

    isInitialized = false;

    return success;
}

bool LaserSensor::isConfigured() const noexcept {
    return ldrPin.getGpioNum() != GPIO_NUM_NC;
}

bool LaserSensor::isReady() const noexcept {
    return isInitialized;
}

void LaserSensor::setThreshold(int desiredThreshold) noexcept {
    threshold = desiredThreshold;
}

int LaserSensor::getThreshold() const noexcept {
    return threshold;
}

std::expected<bool, bool> LaserSensor::sensesLight() const noexcept {
    if (!isInitialized) {
        return std::unexpected(false);
    }

    const auto rawReading = getRawReading();
    if (!rawReading.has_value()) {
        return std::unexpected(false);
    }

    return *rawReading > threshold;
}

std::expected<int, bool> LaserSensor::getRawReading() const noexcept {
    if (!isInitialized) {
        return std::unexpected(false);
    }

    return ldrPin.read().value_or(false);
}
