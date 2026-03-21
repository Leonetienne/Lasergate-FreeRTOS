//
// Created by Leon Etienne on 18.03.26.
//

#include "GpioDigitalWritePin.h"
#include "GpioPinRegister.h"

GpioDigitalWritePin::GpioDigitalWritePin(
    GpioPinRegister& pinRegister,
    IGpio& gpio,
    gpio_num_t pinNum
    ) :
    pinRegister(pinRegister),
    gpio(gpio),
    pinNum { pinNum },
    ready { false },
    currentState { false }
{ }

GpioDigitalWritePin::GpioDigitalWritePin(GpioDigitalWritePin&& other) noexcept :
    pinRegister { other.pinRegister },
    gpio { other.gpio },
    pinNum { other.pinNum },
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
    // Fail if pin is already ready
    if (ready) {
        return false;
    }

    // Attempt to bind the pin
    if (!pinRegister.bindPin(pinNum)) {
        return false;
    }

    // If success, book it/
    if (gpio.gpioResetPin(pinNum) != ESP_OK) {
        pinRegister.freePin(pinNum);
        return false;
    }
    if (gpio.gpioSetDirection(pinNum, GPIO_MODE_OUTPUT) != ESP_OK) {
        pinRegister.freePin(pinNum);
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

    pinRegister.freePin(pinNum);

    ready = false;
    return true;
}

bool GpioDigitalWritePin::setState(PIN_STATE_DIGITAL state) noexcept {
    if (!ready) {
        return false;
    }

    if (gpio.gpioSetlevel(pinNum, static_cast<uint32_t>(state)) != ESP_OK) {
        return false;
    }

    currentState = state;

    return true;
}
