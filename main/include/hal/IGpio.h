//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_IGPIO_H
#define LASERGATE_V2_IGPIO_H

#include "compat/gpio_num_t.h"
#include "compat/gpio_mode_t.h"
#include "compat/esp_err_t.h"
#include <cstdint>

/**
 * Interface class to interact with gpio pins, for output only
 */
class IGpio {
public:
    virtual ~IGpio() = default;

    /**
     * Calls gpio_reset_pin
     */
    virtual esp_err_t gpioResetPin(const gpio_num_t pinNum) noexcept = 0;

    /**
     * Sets a gpio-pins direction (in/out) (in currently not supported)
     */
    virtual esp_err_t gpioSetDirection(const gpio_num_t pinNum, gpio_mode_t pinMode) noexcept = 0;

    /**
     * Sets a gpio-pins output level
     */
    virtual esp_err_t gpioSetLevel(const gpio_num_t pinNum, uint32_t level) noexcept = 0;
};


#endif //LASERGATE_V2_IGPIO_H
