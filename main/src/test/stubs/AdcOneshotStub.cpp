//
// Created by Leon Etienne on 22.03.26.
//

#include "test/stubs/AdcOneshotStub.h"

AdcOneshotStub::AdcOneshotStub(const adc_unit_t adcUnit) noexcept :
    IAdcOneshot(adcUnit)
{
    this->ready = false;
}

AdcOneshotStub::AdcOneshotStub(AdcOneshotStub&& other) noexcept :
    IAdcOneshot(std::move(other))
{
    this->registeredChannels = std::move(other.registeredChannels);
    this->inputValueMap = std::move(other.inputValueMap);
}

esp_err_t AdcOneshotStub::initialize() noexcept {
    this->ready = true;
    return ESP_OK;
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

std::expected<uint16_t, esp_err_t> AdcOneshotStub::readChannel(const adc_channel_t adcChannel) const noexcept {
    if (!isAdcChannelOnCurrentUnit(adcChannel)) {
        return std::unexpected(ESP_ERR_INVALID_ARG);
    }

    if (!registeredChannels.contains(adcChannel)) {
        return std::unexpected(ESP_ERR_INVALID_STATE);
    }

    if (!inputValueMap.contains(adcChannel)) {
        return 0;
    }
    return inputValueMap.at(adcChannel);
}

void AdcOneshotStub::test_setChannelValue(const adc_channel_t adc_channel, uint16_t value) noexcept {
    inputValueMap[adc_channel] = value;
}
