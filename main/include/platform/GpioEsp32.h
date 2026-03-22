//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_GPIOESP32_H
#define LASERGATE_V2_GPIOESP32_H

#include "hal/IGpio.h"

/**
 * Prod impl of IGpio
 */
class GpioEsp32 : public IGpio {
public:
    esp_err_t gpioResetPin(const gpio_num_t pinNum) noexcept override;
    esp_err_t gpioSetDirection(const gpio_num_t pinNum, const gpio_mode_t pinMode) noexcept override;
    esp_err_t gpioSetLevel(const gpio_num_t pinNum, const uint32_t level) noexcept override;
};


#endif //LASERGATE_V2_GPIOESP32_H
