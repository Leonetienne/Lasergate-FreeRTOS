//
// Created by Leon Etienne on 18.03.26.
//

#include "GpioPinRegister.h"

bool GpioPinRegister::isPinBound(gpio_num_t pin) const noexcept {
    return usedPins.find(pin) != usedPins.end();
}

bool GpioPinRegister::bindPin(gpio_num_t pin) noexcept {
    if (isPinBound(pin)) {
        return false;
    }

    usedPins.insert(pin);
    return true;
}

bool GpioPinRegister::freePin(gpio_num_t pin) noexcept {
    if (!isPinBound(pin)) {
        return false;
    }

    usedPins.erase(pin);
    return true;
}

