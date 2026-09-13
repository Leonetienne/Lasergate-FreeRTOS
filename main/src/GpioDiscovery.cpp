#include "GpioDiscovery.h"
#include "hal/AdcGpioMapping.h"
#include "hal/BoardReservedPins.h"

namespace {

GpioDiscovery::PinArray computeAdcPins(const adc_unit_t unit) noexcept {
    GpioDiscovery::PinArray pins{};
    std::size_t count = 0;
    for (const gpio_num_t pin : BoardReservedPins::ALL_PINS) {
        if (BoardReservedPins::isReserved(pin)) {
            continue;
        }
        const auto channel = AdcGpioMapping::gpioToChannel(pin);
        if (channel.has_value() && channel->first == unit && count < pins.size()) {
            pins[count++] = pin;
        }
    }
    return pins;
}

GpioDiscovery::PinArray computeFreeOutputPins() noexcept {
    GpioDiscovery::PinArray pins{};
    std::size_t count = 0;
    for (const gpio_num_t pin : BoardReservedPins::ALL_PINS) {
        if (BoardReservedPins::isReserved(pin) || BoardReservedPins::isInputOnly(pin)) {
            continue;
        }
        const auto channel = AdcGpioMapping::gpioToChannel(pin);
        if (channel.has_value() && (channel->first == ADC_UNIT_1 || channel->first == ADC_UNIT_2)) {
            continue; // covered by readAdc1()/readAdc2()
        }
        if (count < pins.size()) {
            pins[count++] = pin;
        }
    }
    return pins;
}

}

GpioDiscovery::GpioDiscovery(
    const GpioPinRegister& pinRegister,
    IGpio& i_gpio,
    IAdcOneshot& i_adcOneshot1,
    IAdcOneshot& i_adcOneshot2
) noexcept :
    pinRegister(pinRegister),
    i_gpio(i_gpio),
    i_adcOneshot1(i_adcOneshot1),
    i_adcOneshot2(i_adcOneshot2)
{ }

void GpioDiscovery::begin() noexcept {
    for (const auto& pin : getAdc1Pins()) {
        if (!pin.has_value()) {
            continue;
        }
        const auto channel = AdcGpioMapping::gpioToChannel(*pin);
        if (!channel.has_value()) {
            continue;
        }
        // re-registering a channel Gate already set up is harmless, same fixed atten/width
        i_adcOneshot1.registerChannel(channel->second);
    }

    for (const auto& pin : getAdc2Pins()) {
        if (!pin.has_value()) {
            continue;
        }
        const auto channel = AdcGpioMapping::gpioToChannel(*pin);
        if (!channel.has_value()) {
            continue;
        }
        i_adcOneshot2.registerChannel(channel->second);
    }

    outputLevels.clear();
    borrowedPins.clear();
    for (const auto& pin : getFreeOutputPins()) {
        if (!pin.has_value()) {
            continue;
        }
        if (pinRegister.isPinBound(*pin)) {
            borrowedPins.insert(*pin);
        } else {
            i_gpio.gpioSetDirection(*pin, GPIO_MODE_OUTPUT);
        }
        i_gpio.gpioSetLevel(*pin, 0);
        outputLevels[*pin] = false;
    }
}

void GpioDiscovery::end() noexcept {
    for (const auto& [pin, level] : outputLevels) {
        if (borrowedPins.contains(pin)) {
            i_gpio.gpioSetLevel(pin, 0);
        } else {
            i_gpio.gpioResetPin(pin);
        }
    }
    outputLevels.clear();
    borrowedPins.clear();
}

GpioDiscovery::PinArray GpioDiscovery::getDrivenPins() const noexcept {
    PinArray pins{};
    std::size_t count = 0;
    for (const auto& pin : getFreeOutputPins()) {
        if (pin.has_value() && outputLevels.contains(*pin)) {
            pins[count++] = pin;
        }
    }
    return pins;
}

GpioDiscovery::AdcReadingArray GpioDiscovery::readAdc1() const noexcept {
    AdcReadingArray readings{};
    std::size_t count = 0;
    for (const auto& pin : getAdc1Pins()) {
        if (!pin.has_value()) {
            continue;
        }
        const auto channel = AdcGpioMapping::gpioToChannel(*pin);
        if (!channel.has_value()) {
            continue;
        }
        if (const auto value = i_adcOneshot1.readChannel(channel->second); value.has_value()) {
            readings[count++] = std::pair{*pin, *value};
        }
    }
    return readings;
}

GpioDiscovery::AdcReadingArray GpioDiscovery::readAdc2() const noexcept {
    AdcReadingArray readings{};
    std::size_t count = 0;
    for (const auto& pin : getAdc2Pins()) {
        if (!pin.has_value()) {
            continue;
        }
        const auto channel = AdcGpioMapping::gpioToChannel(*pin);
        if (!channel.has_value()) {
            continue;
        }
        if (const auto value = i_adcOneshot2.readChannel(channel->second); value.has_value()) {
            readings[count++] = std::pair{*pin, *value};
        }
    }
    return readings;
}

bool GpioDiscovery::setPinLevel(const gpio_num_t pin, const bool high) noexcept {
    const auto it = outputLevels.find(pin);
    if (it == outputLevels.end()) {
        return false;
    }
    if (i_gpio.gpioSetLevel(pin, high ? 1 : 0) != ESP_OK) {
        return false;
    }
    it->second = high;
    return true;
}

std::optional<bool> GpioDiscovery::getPinLevel(const gpio_num_t pin) const noexcept {
    const auto it = outputLevels.find(pin);
    return it != outputLevels.end() ? std::optional(it->second) : std::nullopt;
}

const GpioDiscovery::PinArray& GpioDiscovery::getAdc1Pins() noexcept {
    static const PinArray pins = computeAdcPins(ADC_UNIT_1);
    return pins;
}

const GpioDiscovery::PinArray& GpioDiscovery::getAdc2Pins() noexcept {
    static const PinArray pins = computeAdcPins(ADC_UNIT_2);
    return pins;
}

const GpioDiscovery::PinArray& GpioDiscovery::getFreeOutputPins() noexcept {
    static const PinArray pins = computeFreeOutputPins();
    return pins;
}
