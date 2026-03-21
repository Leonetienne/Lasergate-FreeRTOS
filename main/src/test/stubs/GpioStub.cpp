//
// Created by Leon Etienne on 21.03.26.
//

#include "test/stubs/GpioStub.h"

esp_err_t GpioStub::gpioResetPin(gpio_num_t pinNum) noexcept {
    return 0;
}

esp_err_t GpioStub::gpioSetDirection(gpio_num_t pinNum, gpio_mode_t pinMode) noexcept {
    return 0;
}

esp_err_t GpioStub::gpioSetlevel(gpio_num_t pinNum, uint32_t level) noexcept {
    return 0;
}
