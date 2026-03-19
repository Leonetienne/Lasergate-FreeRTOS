//
// Created by Leon Etienne on 18.03.26.
//

#ifndef LASERGATE_V2_GPIOPINREGISTER_H
#define LASERGATE_V2_GPIOPINREGISTER_H

#include <unordered_set>
#include "driver/gpio.h"

/**
 * Singleton class, only initializable by class System.
 * Keeps track of which gpio pins are bound.
 */
class GpioPinRegister {
public:
    /**
     * Will return whether a given gpio pin is already bound
     *
     * @param pin a gpio pin number
     *
     * @returns the pin status
     */
    [[nodiscard]] bool isPinBound(gpio_num_t pin) const noexcept;

    /**
     * Will bind a pin if it is not already bound
     *
     * @param pin a gpio pin number
     *
     * @returns success state
     * */
    bool bindPin(gpio_num_t pin) noexcept;

    /**
     * Will free a pin if it bound
     *
     * @param pin a gpio pin number
     *
     * @returns success state
     * */
    bool freePin(gpio_num_t pin) noexcept;

    static GpioPinRegister& getInstance() noexcept;

    GpioPinRegister(const GpioPinRegister&) = delete;
    GpioPinRegister& operator=(const GpioPinRegister&) = delete;

protected:
    std::unordered_set<gpio_num_t> used_pins;

private:
    GpioPinRegister() = default;
    ~GpioPinRegister() = default;
};


#endif //LASERGATE_V2_GPIOPINREGISTER_H