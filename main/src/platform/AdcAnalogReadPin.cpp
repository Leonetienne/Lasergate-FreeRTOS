//
// Created by Leon Etienne on 3/22/26.
//

#include "platform/AdcAnalogReadPin.h"
#include "hal/AdcGpioMapping.h"

AdcAnalogReadPin::AdcAnalogReadPin(GpioPinRegister &pinRegister, IAdcOneshot &i_adcOneshot, const gpio_num_t pinNum) noexcept :
    pinRegister { pinRegister },
    i_adcOneshot { i_adcOneshot },
    gpioPinNum { pinNum },
    ready { false }
{
}

AdcAnalogReadPin::AdcAnalogReadPin(AdcAnalogReadPin &&other) noexcept :
    pinRegister { other.pinRegister },
    i_adcOneshot { other.i_adcOneshot },
    gpioPinNum { other.gpioPinNum },
    adcUnit { other.adcUnit },
    adcChannel { other.adcChannel },
    ready { other.ready }
{
    other.ready = false;
}

AdcAnalogReadPin::~AdcAnalogReadPin() {
    free();
}

esp_err_t AdcAnalogReadPin::initialize() noexcept {
    // Fail if pin is already ready
    if (ready) {
        return ESP_ERR_INVALID_STATE;
    }

    // Attempt to bind the pin
    if (!pinRegister.bindPin(gpioPinNum)) {
        return ESP_ERR_INVALID_STATE;
    }

    // Retrieve adc unit and adc channel for that pin number
    const auto adcChannelInfo = AdcGpioMapping::gpioToChannel(gpioPinNum);
    if (!adcChannelInfo.has_value()) {
        return adcChannelInfo.error();
    }
    this->adcUnit = adcChannelInfo.value().first;
    this->adcChannel = adcChannelInfo.value().second;

    // Is this adc channel on the correct adc unit?
    if (adcUnit != i_adcOneshot.getAdcUnit()) {
        return ESP_ERR_INVALID_ARG;
    }

    // If success, book it/
    if (const esp_err_t ret = i_adcOneshot.registerChannel(adcChannel); ret != ESP_OK) {
        pinRegister.freePin(gpioPinNum);
        return ret;
    }

    ready = true;
    return ESP_OK;
}

bool AdcAnalogReadPin::free() noexcept {
    if (!ready) {
        return false;
    }

    pinRegister.freePin(gpioPinNum);

    ready = false;
    return true;
}

std::expected<uint16_t, esp_err_t> AdcAnalogReadPin::read() const noexcept {
    if (!i_adcOneshot.isReady()) {
        return std::unexpected(ESP_ERR_INVALID_STATE);
    }

    const auto readResult = i_adcOneshot.readChannel(adcChannel);
    if (!readResult.has_value()) {
        return std::unexpected(readResult.error());
    }

    return readResult.value();
}
