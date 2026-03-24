//
// Created by Leon Etienne on 3/22/26.
//

#include "hal/AdcGpioMapping.h"

std::expected<gpio_num_t, esp_err_t> AdcGpioMapping::channelToGpio(
    const adc_unit_t unit,
    const adc_channel_t channel
) noexcept
{
    switch (unit) {
    case ADC_UNIT_1:
        switch (channel) {
        case ADC_CHANNEL_0: return GPIO_NUM_36;
        case ADC_CHANNEL_1: return GPIO_NUM_37;
        case ADC_CHANNEL_2: return GPIO_NUM_38;
        case ADC_CHANNEL_3: return GPIO_NUM_39;
        case ADC_CHANNEL_4: return GPIO_NUM_32;
        case ADC_CHANNEL_5: return GPIO_NUM_33;
        case ADC_CHANNEL_6: return GPIO_NUM_34;
        case ADC_CHANNEL_7: return GPIO_NUM_35;
        default:
            return std::unexpected(ESP_ERR_INVALID_ARG);
        }

    case ADC_UNIT_2:
        switch (channel) {
        case ADC_CHANNEL_0: return GPIO_NUM_4;
        case ADC_CHANNEL_1: return GPIO_NUM_0;
        case ADC_CHANNEL_2: return GPIO_NUM_2;
        case ADC_CHANNEL_3: return GPIO_NUM_15;
        case ADC_CHANNEL_4: return GPIO_NUM_13;
        case ADC_CHANNEL_5: return GPIO_NUM_12;
        case ADC_CHANNEL_6: return GPIO_NUM_14;
        case ADC_CHANNEL_7: return GPIO_NUM_27;
        case ADC_CHANNEL_8: return GPIO_NUM_25;
        case ADC_CHANNEL_9: return GPIO_NUM_26;
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

 switch (gpio) {
    case GPIO_NUM_36: return std::pair{ADC_UNIT_1, ADC_CHANNEL_0};
    case GPIO_NUM_37: return std::pair{ADC_UNIT_1, ADC_CHANNEL_1};
    case GPIO_NUM_38: return std::pair{ADC_UNIT_1, ADC_CHANNEL_2};
    case GPIO_NUM_39: return std::pair{ADC_UNIT_1, ADC_CHANNEL_3};
    case GPIO_NUM_32: return std::pair{ADC_UNIT_1, ADC_CHANNEL_4};
    case GPIO_NUM_33: return std::pair{ADC_UNIT_1, ADC_CHANNEL_5};
    case GPIO_NUM_34: return std::pair{ADC_UNIT_1, ADC_CHANNEL_6};
    case GPIO_NUM_35: return std::pair{ADC_UNIT_1, ADC_CHANNEL_7};

    case GPIO_NUM_4:  return std::pair{ADC_UNIT_2, ADC_CHANNEL_0};
    case GPIO_NUM_0:  return std::pair{ADC_UNIT_2, ADC_CHANNEL_1};
    case GPIO_NUM_2:  return std::pair{ADC_UNIT_2, ADC_CHANNEL_2};
    case GPIO_NUM_15: return std::pair{ADC_UNIT_2, ADC_CHANNEL_3};
    case GPIO_NUM_13: return std::pair{ADC_UNIT_2, ADC_CHANNEL_4};
    case GPIO_NUM_12: return std::pair{ADC_UNIT_2, ADC_CHANNEL_5};
    case GPIO_NUM_14: return std::pair{ADC_UNIT_2, ADC_CHANNEL_6};
    case GPIO_NUM_27: return std::pair{ADC_UNIT_2, ADC_CHANNEL_7};
    case GPIO_NUM_25: return std::pair{ADC_UNIT_2, ADC_CHANNEL_8};
    case GPIO_NUM_26: return std::pair{ADC_UNIT_2, ADC_CHANNEL_9};

    default:
        return std::unexpected(ESP_ERR_INVALID_ARG);
    }
}
