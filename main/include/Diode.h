#ifndef LASERGATE_V2_DIODE_H
#define LASERGATE_V2_DIODE_H

#include "GpioPinRegister.h"
#include "hal/IGpio.h"
#include "platform/GpioDigitalWritePin.h"
#include <expected>

/**
 * Abstract base class for a gpio-driven diode (e.g. a laser diode or LED)
 */
class Diode {
public:
    Diode(GpioPinRegister& pinRegister, IGpio& i_gpio, gpio_num_t pinNum) noexcept;
    Diode(const Diode&) = delete;
    Diode(Diode&&) = delete;
    Diode& operator=(const Diode&) = delete;
    Diode& operator=(Diode&&) = delete;
    virtual ~Diode() noexcept = 0;

    /**
     * Will initialize the diode
     * @return Success state
     */
    virtual bool initialize() noexcept;

    /**
     * Will free all resources acquired by this object
     * @return Success state
     */
    virtual bool free() noexcept;

    /**
     * @return Whether this diode is ready for operation
     */
    [[nodiscard]] virtual bool isReady() const noexcept;

    /**
     * @return Whether an actual GPIO pin is used instead of NC
     */
    [[nodiscard]] bool isConfigured() const noexcept;

    /**
     * Turns the diode on.
     * Idempotent.
     * @return Success state
     */
    virtual bool turnOn() noexcept;

    /**
     * Turns the diode off.
     * Idempotent.
     * @return Success state
     */
    virtual bool turnOff() noexcept;

    /**
     * Turns the diode on or off.
     * Idempotent.
     * @return Success state
     */
    virtual bool setPowerState(bool desiredPowerState) noexcept;

    /**
     * @return The current state of the diode or unexpected<false> if uninitialized
     */
    [[nodiscard]] virtual std::expected<bool, bool> getPowerState() const noexcept;

protected:
    bool isInitialized = false;

    GpioDigitalWritePin gpioPin;
    bool powerState = false;
};


#endif //LASERGATE_V2_DIODE_H
