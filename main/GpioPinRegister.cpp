//
// Created by Leon Etienne on 18.03.26.
//

#include "GpioPinRegister.h"

bool GpioPinRegister::isPinBound(gpio_num_t pin) const noexcept {
    return used_pins.find(pin) != used_pins.end();
}

bool GpioPinRegister::bindPin(gpio_num_t pin) noexcept {
    if (isPinBound(pin)) {
        return false;
    }

    used_pins.insert(pin);
    return true;
}

bool GpioPinRegister::freePin(gpio_num_t pin) noexcept {
    if (!isPinBound(pin)) {
        return false;
    }

    used_pins.erase(pin);
    return true;
}

GpioPinRegister& GpioPinRegister::getInstance() noexcept {
    static GpioPinRegister instance;
    return instance;
}
