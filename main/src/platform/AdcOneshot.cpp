#include "../../include/platform/AdcOneshot.h"
#include "esp_adc/adc_oneshot.h"

AdcOneshot::AdcOneshot(const adc_unit_t adcUnit) noexcept :
    IAdcOneshot(adcUnit),
    adcHandle{}
{
}

AdcOneshot::AdcOneshot(AdcOneshot &&other) noexcept :
    IAdcOneshot(std::move(other)),
    adcHandle{}
{
}

esp_err_t AdcOneshot::initialize() noexcept
{
    if (this->ready) {
        return ESP_ERR_INVALID_STATE;
    }

    adc_oneshot_unit_init_cfg_t adc_init_cfg = {};
    adc_init_cfg.unit_id = this->adcUnit;

    if (const esp_err_t err = adc_oneshot_new_unit(&adc_init_cfg, &this->adcHandle); err != ESP_OK)
    {
        return err;
    }

    this->ready = true;

    return ESP_OK;
}

esp_err_t AdcOneshot::registerChannel(adc_channel_t adcChannel) noexcept
{
    if (this->readyChannels.contains(adcChannel)) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_oneshot_chan_cfg_t adc_chan_cfg = {};
    adc_chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    adc_chan_cfg.atten = ADC_ATTEN_DB_12;

    if (const esp_err_t err = adc_oneshot_config_channel(this->adcHandle, adcChannel, &adc_chan_cfg); err != ESP_OK)
    {
        return err;
    }

    this->readyChannels.emplace(adcChannel);

    return ESP_OK;
}

std::expected<int, esp_err_t> AdcOneshot::readChannel(adc_channel_t adcChannel) const noexcept
{
    int raw = 0;
    if (const esp_err_t err = adc_oneshot_read(this->adcHandle, adcChannel, &raw); err != ESP_OK) {
        return err;
    }
    else {
        return raw;
    }
}
