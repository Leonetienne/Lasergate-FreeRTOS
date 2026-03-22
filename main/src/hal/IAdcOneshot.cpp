//
// Created by Leon Etienne on 22.03.26.
//

#include "hal/IAdcOneshot.h"
#include "hal/AdcGpioMapping.h"

IAdcOneshot::IAdcOneshot(const adc_unit_t adcUnit) noexcept :
    adcUnit { adcUnit }
{
}

IAdcOneshot::IAdcOneshot(IAdcOneshot &&other) noexcept :
    adcUnit { other.adcUnit },
    ready { other.ready }
{
    other.ready = false;
}

bool IAdcOneshot::isAdcChannelOnCurrentUnit(const adc_channel_t adcChannel) const noexcept {
    switch (adcUnit) {
        case ADC_UNIT_1:
           return adcChannel <= ADC_CHANNEL_7;

        case ADC_UNIT_2:
            return adcChannel >= ADC_CHANNEL_8 && adcChannel <= ADC_CHANNEL_10;

        default:
            return false;
    }
}
