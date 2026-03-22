//
// Created by Leon Etienne on 22.03.26.
//

#include "test/stubs/AdcOneshotStub.h"

AdcOneshotStub::AdcOneshotStub(const adc_unit_t adcUnit) noexcept :
    IAdcOneshot(adcUnit)
{
    this->ready = true;
}

AdcOneshotStub::AdcOneshotStub(AdcOneshotStub&& other) noexcept :
    IAdcOneshot(std::move(other))
{
    this->registeredChannels = std::move(other.registeredChannels);
    this->inputValueMap = std::move(other.inputValueMap);
}

esp_err_t AdcOneshotStub::registerChannel(const adc_channel_t adcChannel) noexcept {
    if (!isAdcChannelOnCurrentUnit(adcChannel)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (registeredChannels.contains(adcChannel)) {
        return ESP_ERR_INVALID_STATE;
    }

    registeredChannels.insert(adcChannel);
    return ESP_OK;
}

esp_err_t AdcOneshotStub::readChannel(const adc_channel_t adcChannel, int& buffer) const noexcept {
    if (!isAdcChannelOnCurrentUnit(adcChannel)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!registeredChannels.contains(adcChannel)) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!inputValueMap.contains(adcChannel)) {
        buffer = 0;
    }
    else {
        buffer = inputValueMap.at(adcChannel);
    }
    return ESP_OK;
}

void AdcOneshotStub::test_setChannelValue(const adc_channel_t adc_channel, int value) noexcept {
    inputValueMap[adc_channel] = value;
}
