//
// Created by Leon Etienne on 21.03.26.
//

#include "platform/GpioEsp32.h"
#include "driver/gpio.h"

esp_err_t GpioEsp32::gpioResetPin(gpio_num_t pinNum) noexcept {
    return gpio_reset_pin(pinNum);
}

esp_err_t GpioEsp32::gpioSetDirection(gpio_num_t pinNum, gpio_mode_t pinMode) noexcept {
    return gpio_set_direction(pinNum, pinMode);
}

esp_err_t GpioEsp32::gpioSetLevel(gpio_num_t pinNum, uint32_t level) noexcept {
    return gpio_set_level(pinNum, level);
}
