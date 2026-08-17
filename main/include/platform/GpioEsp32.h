//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_GPIOESP32_H
#define LASERGATE_V2_GPIOESP32_H

#include "hal/IGpio.h"

/**
 * Impl of IGpio for esp32
 */
class GpioEsp32 : public IGpio {
public:
    GpioEsp32() = default;
    GpioEsp32(const GpioEsp32&) = delete;
    GpioEsp32& operator=(const GpioEsp32&) = delete;
    GpioEsp32(GpioEsp32&&) noexcept;
    GpioEsp32& operator=(GpioEsp32&&) = delete;

    /**
     * Calls gpio_reset_pin
     */
    esp_err_t gpioResetPin(const gpio_num_t pinNum) noexcept override;

    /**
    * Calls gpio_set_direction
    */
    esp_err_t gpioSetDirection(const gpio_num_t pinNum, const gpio_mode_t pinMode) noexcept override;

    /**
     * Calls gpio_set_level
     */
    esp_err_t gpioSetLevel(const gpio_num_t pinNum, const uint32_t level) noexcept override;

    /**
     * Calls gpio_get_level
     */
    uint32_t gpioGetLevel(const gpio_num_t pinNum) noexcept override;
};


#endif //LASERGATE_V2_GPIOESP32_H
