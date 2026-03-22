//
// Created by Leon Etienne on 3/22/26.
//

#include "platform/AdcAnalogReadPin.h"
#include "hal/AdcGpioMapping.h"

AdcAnalogReadPin::AdcAnalogReadPin(GpioPinRegister &pinRegister, IAdcOneshot &adcOS, const gpio_num_t pinNum) noexcept :
    pinRegister { pinRegister },
    adcOneshot { adcOS },
    gpioPinNum { pinNum },
    ready { false }
{
}

AdcAnalogReadPin::AdcAnalogReadPin(AdcAnalogReadPin &&other) noexcept :
    pinRegister { other.pinRegister },
    adcOneshot { other.adcOneshot },
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
    if (const esp_err_t res = AdcGpioMapping::gpioToChannel(
        gpioPinNum,
        adcUnit,
        adcChannel
    ); res != ESP_OK) {
        return res;
    }

    // Is this adc channel on the correct adc unit?
    if (adcUnit != adcOneshot.getAdcUnit()) {
        return ESP_ERR_INVALID_ARG;
    }

    // If success, book it/
    if (const esp_err_t ret = adcOneshot.registerChannel(adcChannel); ret != ESP_OK) {
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

std::expected<int, esp_err_t> AdcAnalogReadPin::read() const noexcept {
    if (!adcOneshot.isReady()) {
        return std::unexpected(ESP_ERR_INVALID_STATE);
    }

    int buf;
    if (const esp_err_t ret = adcOneshot.readChannel(adcChannel, buf); ret != ESP_OK) {
        return std::unexpected(ret);
    }

    return buf;
}
