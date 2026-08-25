//
// Created by Leon Etienne on 3/22/26.
//

#include "hal/AdcGpioMapping.h"

std::expected<gpio_num_t, esp_err_t> AdcGpioMapping::channelToGpio(
    const adc_unit_t unit,
    const adc_channel_t channel
) noexcept
{
    // esp32-s3: adc1 = gpio1-10 (ch0-9), adc2 = gpio11-20 (ch0-9)
    switch (unit) {
    case ADC_UNIT_1:
        switch (channel) {
        case ADC_CHANNEL_0: return GPIO_NUM_1;
        case ADC_CHANNEL_1: return GPIO_NUM_2;
        case ADC_CHANNEL_2: return GPIO_NUM_3;
        case ADC_CHANNEL_3: return GPIO_NUM_4;
        case ADC_CHANNEL_4: return GPIO_NUM_5;
        case ADC_CHANNEL_5: return GPIO_NUM_6;
        case ADC_CHANNEL_6: return GPIO_NUM_7;
        case ADC_CHANNEL_7: return GPIO_NUM_8;
        case ADC_CHANNEL_8: return GPIO_NUM_9;
        case ADC_CHANNEL_9: return GPIO_NUM_10;
        default:
            return std::unexpected(ESP_ERR_INVALID_ARG);
        }

    case ADC_UNIT_2:
        switch (channel) {
        case ADC_CHANNEL_0: return GPIO_NUM_11;
        case ADC_CHANNEL_1: return GPIO_NUM_12;
        case ADC_CHANNEL_2: return GPIO_NUM_13;
        case ADC_CHANNEL_3: return GPIO_NUM_14;
        case ADC_CHANNEL_4: return GPIO_NUM_15;
        case ADC_CHANNEL_5: return GPIO_NUM_16;
        case ADC_CHANNEL_6: return GPIO_NUM_17;
        case ADC_CHANNEL_7: return GPIO_NUM_18;
        case ADC_CHANNEL_8: return GPIO_NUM_19;
        case ADC_CHANNEL_9: return GPIO_NUM_20;
        default:
            return std::unexpected(ESP_ERR_INVALID_ARG);
        }

    default:
        return std::unexpected(ESP_ERR_INVALID_ARG);
    }
}

std::expected<
    std::pair<adc_unit_t, adc_channel_t>,
    esp_err_t
> AdcGpioMapping::gpioToChannel(
    const gpio_num_t gpio
) noexcept
{

 // esp32-s3: adc1 = gpio1-10 (ch0-9), adc2 = gpio11-20 (ch0-9)
 switch (gpio) {
    case GPIO_NUM_1: return std::pair{ADC_UNIT_1, ADC_CHANNEL_0};
    case GPIO_NUM_2: return std::pair{ADC_UNIT_1, ADC_CHANNEL_1};
    case GPIO_NUM_3: return std::pair{ADC_UNIT_1, ADC_CHANNEL_2};
    case GPIO_NUM_4: return std::pair{ADC_UNIT_1, ADC_CHANNEL_3};
    case GPIO_NUM_5: return std::pair{ADC_UNIT_1, ADC_CHANNEL_4};
    case GPIO_NUM_6: return std::pair{ADC_UNIT_1, ADC_CHANNEL_5};
    case GPIO_NUM_7: return std::pair{ADC_UNIT_1, ADC_CHANNEL_6};
    case GPIO_NUM_8: return std::pair{ADC_UNIT_1, ADC_CHANNEL_7};
    case GPIO_NUM_9: return std::pair{ADC_UNIT_1, ADC_CHANNEL_8};
    case GPIO_NUM_10: return std::pair{ADC_UNIT_1, ADC_CHANNEL_9};

    case GPIO_NUM_11: return std::pair{ADC_UNIT_2, ADC_CHANNEL_0};
    case GPIO_NUM_12: return std::pair{ADC_UNIT_2, ADC_CHANNEL_1};
    case GPIO_NUM_13: return std::pair{ADC_UNIT_2, ADC_CHANNEL_2};
    case GPIO_NUM_14: return std::pair{ADC_UNIT_2, ADC_CHANNEL_3};
    case GPIO_NUM_15: return std::pair{ADC_UNIT_2, ADC_CHANNEL_4};
    case GPIO_NUM_16: return std::pair{ADC_UNIT_2, ADC_CHANNEL_5};
    case GPIO_NUM_17: return std::pair{ADC_UNIT_2, ADC_CHANNEL_6};
    case GPIO_NUM_18: return std::pair{ADC_UNIT_2, ADC_CHANNEL_7};
    case GPIO_NUM_19: return std::pair{ADC_UNIT_2, ADC_CHANNEL_8};
    case GPIO_NUM_20: return std::pair{ADC_UNIT_2, ADC_CHANNEL_9};

    default:
        return std::unexpected(ESP_ERR_INVALID_ARG);
    }
}
