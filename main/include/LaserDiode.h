#ifndef LASERGATE_V2_LASERDIODE_H
#define LASERGATE_V2_LASERDIODE_H

#include "platform/GpioDigitalWritePin.h"
#include <expected>

class LaserDiode {
public:
    LaserDiode(GpioDigitalWritePin& gpioPin) noexcept;
    LaserDiode(const LaserDiode&) = delete;
    LaserDiode(LaserDiode&&) = delete;
    LaserDiode& operator=(const LaserDiode&) = delete;
    LaserDiode& operator=(LaserDiode&&) = delete;
    ~LaserDiode() noexcept;

    /**
     * Will initialize the laser diode
     * @return Success state
     */
    bool initialize() noexcept;

    /**
     * Will free all resources acquired by this object
     * @return Success state
     */
    bool free() noexcept;

    /**
     * @return Whether this laser diode is ready for operation
     */
    [[nodiscard]] bool isReady() const noexcept;

    /**
     * Turns the laser diode on.
     * Idempotent.
     * @return Success state
     */
    bool turnOn() const noexcept;

    /**
     * Turns the laser diode off.
     * Idempotent.
     * @return Success state
     */
    bool turnOff() const noexcept;

    /**
     * Turns the laser diode on or off.
     * Idempotent.
     * @return Success state
     */
    bool setPowerState(bool desiredPowerState) noexcept;

    /**
     * @return The current state of the laser diode or unexpected<false> if uninitialized
     */
    [[nodiscard]] std::expected<bool, bool> getPowerState() const noexcept;

private:
    bool isInitialized = false;

    GpioDigitalWritePin& gpioPin;
    bool powerState = false;
};


#endif //LASERGATE_V2_LASERDIODE_H
