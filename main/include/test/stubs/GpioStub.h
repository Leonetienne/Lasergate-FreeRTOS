//
// Created by Leon Etienne on 21.03.26.
//

#ifndef LASERGATE_V2_GPIOSTUB_H
#define LASERGATE_V2_GPIOSTUB_H

#include "hal/IGpio.h"
#include <unordered_map>

class GpioStub : public IGpio {
public:
    GpioStub() = default;
    GpioStub(const GpioStub&) = delete;
    GpioStub(GpioStub&& other) noexcept;
    GpioStub& operator=(const GpioStub&) = delete;
    ~GpioStub() override = default;

    /**
     * Will reset a pins state
     */
    esp_err_t gpioResetPin(const gpio_num_t pinNum) noexcept override;

    /**
     * Will set a pins direction (in/out) (in currently not supported)
     */
    esp_err_t gpioSetDirection(const gpio_num_t pinNum, const gpio_mode_t pinMode) noexcept override;

    /**
     * Will set a pins output level
     */
    esp_err_t gpioSetLevel(const gpio_num_t pinNum, const uint32_t level) noexcept override;

    /* Unit test interrogoters */
    [[ nodiscard ]] gpio_mode_t test_gpioGetMode(const gpio_num_t pinNum) noexcept;
    [[ nodiscard ]] uint32_t test_gpioGetLevel(const gpio_num_t pinNum) noexcept;

private:
    std::unordered_map<gpio_num_t, uint32_t> pinLevelMap;
    std::unordered_map<gpio_num_t, gpio_mode_t> pinDirectionMap;
};


#endif //LASERGATE_V2_GPIOSTUB_H
