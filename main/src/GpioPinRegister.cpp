//
// Created by Leon Etienne on 18.03.26.
//

#include "../include/GpioPinRegister.h"

bool GpioPinRegister::isPinBound(const gpio_num_t pin) const noexcept {
    return usedPins.contains(pin);
}

bool GpioPinRegister::bindPin(const gpio_num_t pin) noexcept {
    if (isPinBound(pin)) {
        return false;
    }

    usedPins.insert(pin);
    return true;
}

bool GpioPinRegister::freePin(const gpio_num_t pin) noexcept {
    if (!isPinBound(pin)) {
        return false;
    }

    usedPins.erase(pin);
    return true;
}

