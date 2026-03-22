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
    esp_err_t gpioResetPin(gpio_num_t pinNum) noexcept override;
    esp_err_t gpioSetDirection(gpio_num_t pinNum, gpio_mode_t pinMode) noexcept override;
    esp_err_t gpioSetLevel(gpio_num_t pinNum, uint32_t level) noexcept override;
};


#endif //LASERGATE_V2_GPIOESP32_H
