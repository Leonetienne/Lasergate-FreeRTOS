//
// Created by Leon Etienne on 18.03.26.
//

#include "GpioDigitalWritePin.h"
#include "GpioPinRegister.h"

GpioDigitalWritePin::GpioDigitalWritePin(gpio_num_t pin) :
    pin { pin },
    ready { false },
    currentState { false }
{ }

GpioDigitalWritePin::GpioDigitalWritePin(GpioDigitalWritePin&& other) noexcept :
    pin { other.pin },
    ready { other.ready },
    currentState { other.currentState }
{
    other.ready = false;
    other.currentState = PIN_STATE_DIGITAL::LOW;
}

GpioDigitalWritePin::~GpioDigitalWritePin() {
    if (ready) {
        free();
    }
}

bool GpioDigitalWritePin::initialize() noexcept {
    // Attempt to bind the pin
    if (!GpioPinRegister::getInstance().bindPin(pin)) {
        return false;
    }

    // If success, book it/
    if (gpio_reset_pin(pin) != ESP_OK) {
        GpioPinRegister::getInstance().freePin(pin);
        return false;
    }
    if (gpio_set_direction(pin, GPIO_MODE_OUTPUT) != ESP_OK) {
        GpioPinRegister::getInstance().freePin(pin);
        return false;
    }

    ready = true;
    return true;
}

bool GpioDigitalWritePin::free() noexcept {
    if (!ready) {
        return false;
    }

    // Reset the pin state to LOW
    setState(PIN_STATE_DIGITAL::LOW);

    GpioPinRegister::getInstance().freePin(pin);

    ready = false;
    return true;
}

bool GpioDigitalWritePin::setState(PIN_STATE_DIGITAL state) noexcept {
    if (!ready) {
        return false;
    }

    if (gpio_set_level(pin, static_cast<int>(state)) != ESP_OK) {
        return false;
    }

    currentState = state;

    return true;
}
