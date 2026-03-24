//
// Created by Leon Etienne on 21.03.26.
//

#include "test/stubs/GpioStub.h"

GpioStub::GpioStub(GpioStub&& other) noexcept {
    this->pinLevelMap = std::move(other.pinLevelMap);
    this->pinDirectionMap = std::move(other.pinDirectionMap);
}

esp_err_t GpioStub::gpioResetPin(const gpio_num_t pinNum) noexcept {
    return ESP_OK;
}

esp_err_t GpioStub::gpioSetDirection(const gpio_num_t pinNum, const gpio_mode_t pinMode) noexcept {
    if (pinMode != GPIO_MODE_OUTPUT) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    pinDirectionMap[pinNum] = pinMode;
    return ESP_OK;
}

esp_err_t GpioStub::gpioSetLevel(const gpio_num_t pinNum, const uint32_t level) noexcept {
    pinLevelMap[pinNum] = level;
    return ESP_OK;
}

gpio_num_t GpioStub::test_gpioGetMode(const gpio_num_t pinNum) noexcept {
    if (!pinDirectionMap.contains(pinNum)) {
        return GPIO_MODE_DISABLE;
    }
    return pinDirectionMap[pinNum];
}

uint32_t GpioStub::test_gpioGetLevel(const gpio_num_t pinNum) noexcept {
    if (!pinLevelMap.contains(pinNum)) {
        return 0;
    }
    return pinLevelMap[pinNum];
}
