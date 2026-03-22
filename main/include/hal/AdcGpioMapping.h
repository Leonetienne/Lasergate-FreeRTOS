//
// Created by nixmage on 3/22/26.
//

#ifndef LASERGATE_TESTS_ADCGPIOMAPPING_H
#define LASERGATE_TESTS_ADCGPIOMAPPING_H

#include "compat/esp_err_t.h"
#include "compat/gpio_num_t.h"
#include "compat/adc_channel_t.h"
#include "compat/adc_unit_t.h"

struct AdcGpioMapping
{
    static esp_err_t channelToGpio(
        const adc_unit_t unit,
        const adc_channel_t channel,
        gpio_num_t& outGpio) noexcept;

    static esp_err_t gpioToChannel(
        const gpio_num_t gpio,
        adc_unit_t& outUnit,
        adc_channel_t& outChannel) noexcept;
};

#endif //LASERGATE_TESTS_ADCGPIOMAPPING_H