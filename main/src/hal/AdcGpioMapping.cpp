//
// Created by Leon Etienne on 3/22/26.
//

#include "hal/AdcGpioMapping.h"

esp_err_t AdcGpioMapping::channelToGpio(
    const adc_unit_t unit,
    const adc_channel_t channel,
    gpio_num_t& outGpio) noexcept
{
    switch (unit) {
    case ADC_UNIT_1:
        switch (channel) {
        case ADC_CHANNEL_0: outGpio = 36; return ESP_OK;
        case ADC_CHANNEL_1: outGpio = 37; return ESP_OK;
        case ADC_CHANNEL_2: outGpio = 38; return ESP_OK;
        case ADC_CHANNEL_3: outGpio = 39; return ESP_OK;
        case ADC_CHANNEL_4: outGpio = 32; return ESP_OK;
        case ADC_CHANNEL_5: outGpio = 33; return ESP_OK;
        case ADC_CHANNEL_6: outGpio = 34; return ESP_OK;
        case ADC_CHANNEL_7: outGpio = 35; return ESP_OK;
        default: return -1;
        }

    case ADC_UNIT_2:
        switch (channel) {
        case ADC_CHANNEL_0: outGpio = 4;  return ESP_OK;
        case ADC_CHANNEL_1: outGpio = 0;  return ESP_OK;
        case ADC_CHANNEL_2: outGpio = 2;  return ESP_OK;
        case ADC_CHANNEL_3: outGpio = 15; return ESP_OK;
        case ADC_CHANNEL_4: outGpio = 13; return ESP_OK;
        case ADC_CHANNEL_5: outGpio = 12; return ESP_OK;
        case ADC_CHANNEL_6: outGpio = 14; return ESP_OK;
        case ADC_CHANNEL_7: outGpio = 27; return ESP_OK;
        case ADC_CHANNEL_8: outGpio = 25; return ESP_OK;
        case ADC_CHANNEL_9: outGpio = 26; return ESP_OK;
        default: return ESP_ERR_INVALID_ARG;
        }

    default:
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t AdcGpioMapping::gpioToChannel(
    const gpio_num_t gpio,
    adc_unit_t& outUnit,
    adc_channel_t& outChannel) noexcept
{
    switch (gpio) {
    case 36: outUnit = ADC_UNIT_1; outChannel = ADC_CHANNEL_0; return ESP_OK;
    case 37: outUnit = ADC_UNIT_1; outChannel = ADC_CHANNEL_1; return ESP_OK;
    case 38: outUnit = ADC_UNIT_1; outChannel = ADC_CHANNEL_2; return ESP_OK;
    case 39: outUnit = ADC_UNIT_1; outChannel = ADC_CHANNEL_3; return ESP_OK;
    case 32: outUnit = ADC_UNIT_1; outChannel = ADC_CHANNEL_4; return ESP_OK;
    case 33: outUnit = ADC_UNIT_1; outChannel = ADC_CHANNEL_5; return ESP_OK;
    case 34: outUnit = ADC_UNIT_1; outChannel = ADC_CHANNEL_6; return ESP_OK;
    case 35: outUnit = ADC_UNIT_1; outChannel = ADC_CHANNEL_7; return ESP_OK;

    case 4:  outUnit = ADC_UNIT_2; outChannel = ADC_CHANNEL_0; return ESP_OK;
    case 0:  outUnit = ADC_UNIT_2; outChannel = ADC_CHANNEL_1; return ESP_OK;
    case 2:  outUnit = ADC_UNIT_2; outChannel = ADC_CHANNEL_2; return ESP_OK;
    case 15: outUnit = ADC_UNIT_2; outChannel = ADC_CHANNEL_3; return ESP_OK;
    case 13: outUnit = ADC_UNIT_2; outChannel = ADC_CHANNEL_4; return ESP_OK;
    case 12: outUnit = ADC_UNIT_2; outChannel = ADC_CHANNEL_5; return ESP_OK;
    case 14: outUnit = ADC_UNIT_2; outChannel = ADC_CHANNEL_6; return ESP_OK;
    case 27: outUnit = ADC_UNIT_2; outChannel = ADC_CHANNEL_7; return ESP_OK;
    case 25: outUnit = ADC_UNIT_2; outChannel = ADC_CHANNEL_8; return ESP_OK;
    case 26: outUnit = ADC_UNIT_2; outChannel = ADC_CHANNEL_9; return ESP_OK;

    default:
        return ESP_ERR_NOT_FOUND;
    }
}
