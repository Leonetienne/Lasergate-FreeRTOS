#ifndef LASERGATE_TESTS_LASERSENSOR_H
#define LASERGATE_TESTS_LASERSENSOR_H

#include "GpioPinRegister.h"
#include "hal/IAdcOneshot.h"
#include "platform/AdcAnalogReadPin.h"
#include <expected>

/**
 * Senses whether a laser beam is hitting an LDR
 */
class LaserSensor {
public:
    LaserSensor(GpioPinRegister& pinRegister, IAdcOneshot& i_adcOneshot, gpio_num_t pinNum) noexcept;
    LaserSensor(const LaserSensor&) = delete;
    LaserSensor(LaserSensor&&) = delete;
    LaserSensor& operator=(const LaserSensor&) = delete;
    LaserSensor& operator=(LaserSensor&&) = delete;
    ~LaserSensor() noexcept;

    /**
     * Will initialize the sensor
     * @return Success state
     */
    bool initialize(uint16_t desiredThreshold) noexcept;

    /**
     * Will free all resources acquired by this object
     * @return Success state
     */
    bool free() noexcept;

    /**
     * @return Whether an actual GPIO pin is used instead of NC
     */
    [[nodiscard]] bool isConfigured() const noexcept;


    /**
     * @return Whether this sensor is ready for operation
     */
    [[nodiscard]] bool isReady() const noexcept;

    void setThreshold(uint16_t desiredThreshold) noexcept;
    [[nodiscard]] uint16_t getThreshold() const noexcept;

    /**
     * @return Whether the sensor senses light or unexpected<false> if uninitialized
     */
    [[nodiscard]] std::expected<bool, bool> sensesLight() const noexcept;

    /**
     * @return The raw reading of the LDR or unexpected<false> if uninitialized
     */
    [[nodiscard]] std::expected<uint16_t, bool> getRawReading() const noexcept;

private:
    bool isInitialized = false;

    /**
     * Every adc read value above threshold is treated as HIGH / "laser hit".
     */
    uint16_t threshold = 0;
    AdcAnalogReadPin ldrPin;

};


#endif //LASERGATE_TESTS_LASERSENSOR_H
