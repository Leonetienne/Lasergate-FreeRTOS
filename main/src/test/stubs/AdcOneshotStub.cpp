//
// Created by Leon Etienne on 22.03.26.
//

#include "test/stubs/AdcOneshotStub.h"

AdcOneshotStub::AdcOneshotStub(adc_unit_t adcUnit) noexcept :
    adcUnit { adcUnit },
    ready { true }
{}

AdcOneshotStub::AdcOneshotStub(AdcOneshotStub&& other) noexcept :
    adcUnit( other.adcUnit )
{
    this->registeredChannels = std::move(other.registeredChannels);
    this->inputValueMap = std::move(other.inputValueMap);
    this->ready = other.ready;
    other.ready = false;
}

esp_err_t AdcOneshotStub::registerChannel(adc_channel_t adcChannel) noexcept {
    if (registeredChannels.contains(adcChannel)) {
        return ESP_ERR_INVALID_STATE;
    }
    registeredChannels.insert(adcChannel);
    return ESP_OK;
}

esp_err_t AdcOneshotStub::readChannel(adc_channel_t adcChannel, int& buffer) const noexcept {
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

void AdcOneshotStub::test_setChannelValue(adc_channel_t adc_channel, int value) noexcept {
    inputValueMap[adc_channel] = value;
}
