//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_GPIOMOCK_H
#define LASERGATE_V2_GPIOMOCK_H

#include "hal/IGpio.h"
#include <unordered_map>

class GpioStub : public IGpio {
public:
    ~GpioStub() override = default;
    esp_err_t gpioResetPin(gpio_num_t pinNum) noexcept override;
    esp_err_t gpioSetDirection(gpio_num_t pinNum, gpio_mode_t pinMode) noexcept override;
    esp_err_t gpioSetlevel(gpio_num_t pinNum, uint32_t level) noexcept override;

private:
};


#endif //LASERGATE_V2_GPIOMOCK_H