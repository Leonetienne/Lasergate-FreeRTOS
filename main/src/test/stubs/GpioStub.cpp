//
// Created by Leon Etienne on 21.03.26.
//

#include "test/stubs/GpioStub.h"

GpioStub::GpioStub(GpioStub&& other) noexcept {
    this->pinLevelMap = std::move(other.pinLevelMap);
    this->pinDirectionMap = std::move(other.pinDirectionMap);
}

esp_err_t GpioStub::gpioResetPin(gpio_num_t pinNum) noexcept {
    return ESP_OK;
}

esp_err_t GpioStub::gpioSetDirection(gpio_num_t pinNum, gpio_mode_t pinMode) noexcept {
    pinDirectionMap[pinNum] = pinMode;
    return ESP_OK;
}

esp_err_t GpioStub::gpioSetLevel(gpio_num_t pinNum, uint32_t level) noexcept {
    pinLevelMap[pinNum] = level;
    return ESP_OK;
}

gpio_num_t GpioStub::test_gpioGetMode(gpio_num_t pinNum) noexcept {
    if (!pinDirectionMap.contains(pinNum)) {
        return GPIO_MODE_DISABLE;
    }
    return pinDirectionMap[pinNum];
}

uint32_t GpioStub::test_gpioGetLevel(gpio_num_t pinNum) noexcept {
    if (!pinLevelMap.contains(pinNum)) {
        return 0;
    }
    return pinLevelMap[pinNum];
}
