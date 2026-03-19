//
// Created by Leon Etienne on 18.03.26.
//

#ifndef LASERGATE_V2_GPIOWRITEPIN_H
#define LASERGATE_V2_GPIOWRITEPIN_H
#include "driver/gpio.h"
#include "PinState.h"

/**
 * Class to write or read to a gpio pin
 */
class GpioDigitalWritePin {
public:
    GpioDigitalWritePin(gpio_num_t pin);
    GpioDigitalWritePin(const GpioDigitalWritePin&) = delete;
    GpioDigitalWritePin(GpioDigitalWritePin&& other) noexcept;
    ~GpioDigitalWritePin();
    GpioDigitalWritePin& operator=(const GpioDigitalWritePin&) = delete;

    /**
     * Will initialize the pin
     * @return Success state
     */
    bool initialize() noexcept;

    /**
     * Will unbind the pin and defunc this object
     *
     * @return Success state
     */
    bool free() noexcept;

    /**
     * @return Whether this object is ready to be used
     */
    [[nodiscard]] bool isReady() const noexcept { return ready; };

    /**
     * @return The gpio pin number
     */
    [[nodiscard]] gpio_num_t getGpioNum() const noexcept { return pin; };

    /**
     * @return Whether the pin is currently emitting LOW or HIGH
     */
    [[nodiscard]] PIN_STATE_DIGITAL getState() const noexcept { return currentState; };

    /**
     * Will write a new status to the pin
     *
     * @param state The new pin state
     * @return Success status
     */
    bool setState(PIN_STATE_DIGITAL state) noexcept;

protected:
    const gpio_num_t pin;
    bool ready;
    PIN_STATE_DIGITAL currentState;
};


#endif //LASERGATE_V2_GPIOWRITEPIN_H
