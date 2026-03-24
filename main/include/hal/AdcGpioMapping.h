//
// Created by nixmage on 3/22/26.
//

#ifndef LASERGATE_TESTS_ADCGPIOMAPPING_H
#define LASERGATE_TESTS_ADCGPIOMAPPING_H

#include "compat/esp_err_t.h"
#include "compat/gpio_num_t.h"
#include "compat/adc_channel_t.h"
#include "compat/adc_unit_t.h"

/**
 * Helper struct to translate between adc (units, channels) and gpio pins
 */
struct AdcGpioMapping
{
    /**
     * Will translate an adc unit and an adc channel to a gpio pin number
     */
    static esp_err_t channelToGpio(
        const adc_unit_t unit,
        const adc_channel_t channel,
        gpio_num_t& outGpio) noexcept;

    /**
     * Will translate a gpio pin number to an adc channel and an adc unit
     */
    static esp_err_t gpioToChannel(
        const gpio_num_t gpio,
        adc_unit_t& outUnit,
        adc_channel_t& outChannel) noexcept;
};

#endif //LASERGATE_TESTS_ADCGPIOMAPPING_H