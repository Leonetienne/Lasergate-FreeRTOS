//
// Created by Leon Etienne on 18.03.26.
//

#include "../../include/platform/GpioDigitalWritePin.h"
#include "../../include/GpioPinRegister.h"

GpioDigitalWritePin::GpioDigitalWritePin(
    GpioPinRegister& pinRegister,
    IGpio& i_gpio,
    const gpio_num_t pinNum
    ) :
    pinRegister(pinRegister),
    i_gpio(i_gpio),
    pinNum { pinNum },
    ready { false },
    currentState { false }
{ }

GpioDigitalWritePin::GpioDigitalWritePin(GpioDigitalWritePin&& other) noexcept :
    pinRegister { other.pinRegister },
    i_gpio { other.i_gpio },
    pinNum { other.pinNum },
    ready { other.ready },
    currentState { other.currentState }
{
    other.ready = false;
    other.currentState = PIN_STATE_DIGITAL::LOW;
}

GpioDigitalWritePin& GpioDigitalWritePin::operator=(GpioDigitalWritePin&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (ready) {
        free();
    }

    // pinRegister, i_gpio and pinNum are bound at construction and left untouched here
    ready = other.ready;
    currentState = other.currentState;

    other.ready = false;
    other.currentState = PIN_STATE_DIGITAL::LOW;

    return *this;
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
    if (i_gpio.gpioResetPin(pinNum) != ESP_OK) {
        pinRegister.freePin(pinNum);
        return false;
    }
    if (i_gpio.gpioSetDirection(pinNum, GPIO_MODE_OUTPUT) != ESP_OK) {
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

bool GpioDigitalWritePin::setState(const PIN_STATE_DIGITAL state) noexcept {
    if (!ready) {
        return false;
    }

    if (i_gpio.gpioSetLevel(pinNum, static_cast<uint32_t>(state)) != ESP_OK) {
        return false;
    }

    currentState = state;

    return true;
}
