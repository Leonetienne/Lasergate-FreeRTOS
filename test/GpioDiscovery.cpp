#include <catch2/catch_test_macros.hpp>
#include "GpioDiscovery.h"
#include "hal/AdcGpioMapping.h"
#include "test/stubs/AdcOneshotStub.h"
#include "test/stubs/GpioStub.h"
#include <algorithm>
#include <vector>

namespace {

std::vector<gpio_num_t> presentPins(const GpioDiscovery::PinArray& pins) {
    std::vector<gpio_num_t> result;
    for (const auto& pin : pins) {
        if (pin.has_value()) {
            result.push_back(*pin);
        }
    }
    return result;
}

std::vector<std::pair<gpio_num_t, uint16_t>> presentReadings(const GpioDiscovery::AdcReadingArray& readings) {
    std::vector<std::pair<gpio_num_t, uint16_t>> result;
    for (const auto& reading : readings) {
        if (reading.has_value()) {
            result.push_back(*reading);
        }
    }
    return result;
}

}

TEST_CASE("GpioDiscovery: static pin classification", "[GpioDiscovery]") {
    SECTION("getAdc1Pins is gpio1 and gpio2, the rest of adc1 isn't broken out") {
        const auto& pins = GpioDiscovery::getAdc1Pins();
        const std::vector<gpio_num_t> expected { GPIO_NUM_1, GPIO_NUM_2 };
        REQUIRE(presentPins(pins) == expected);
    }

    SECTION("getAdc2Pins is gpio15 through gpio18") {
        const auto& pins = GpioDiscovery::getAdc2Pins();
        const std::vector<gpio_num_t> expected { GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18 };
        REQUIRE(presentPins(pins) == expected);
    }

    SECTION("getFreeOutputPins excludes reserved, adc1 and adc2 pins") {
        const auto& pins = GpioDiscovery::getFreeOutputPins();
        const std::vector<gpio_num_t> expected {
            GPIO_NUM_21, GPIO_NUM_38, GPIO_NUM_39, GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_42, GPIO_NUM_47, GPIO_NUM_48
        };
        REQUIRE(presentPins(pins) == expected);
    }
}

TEST_CASE("GpioDiscovery: begin/end", "[GpioDiscovery]") {
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    AdcOneshotStub adcStub2(ADC_UNIT_2);
    GpioPinRegister pinRegister{};
    GpioDiscovery discovery(pinRegister, gpioStub, adcStub, adcStub2);

    SECTION("begin configures every free output pin as output, driven low") {
        discovery.begin();

        for (const gpio_num_t pin : presentPins(GpioDiscovery::getFreeOutputPins())) {
            REQUIRE(gpioStub.test_gpioGetMode(pin) == GPIO_MODE_OUTPUT);
            REQUIRE(gpioStub.test_gpioGetLevel(pin) == 0);
            REQUIRE(discovery.getPinLevel(pin) == std::optional(false));
        }
    }

    SECTION("begin registers every adc1 pin's channel") {
        discovery.begin();

        for (const gpio_num_t pin : presentPins(GpioDiscovery::getAdc1Pins())) {
            const auto channel = AdcGpioMapping::gpioToChannel(pin);
            REQUIRE(channel.has_value());
            adcStub.test_setChannelValue(channel->second, 1234);
        }

        const auto readings = presentReadings(discovery.readAdc1());
        REQUIRE(readings.size() == presentPins(GpioDiscovery::getAdc1Pins()).size());
        for (const auto& [pin, value] : readings) {
            (void)pin;
            REQUIRE(value == 1234);
        }
    }

    SECTION("begin registers every adc2 pin's channel") {
        discovery.begin();

        for (const gpio_num_t pin : presentPins(GpioDiscovery::getAdc2Pins())) {
            const auto channel = AdcGpioMapping::gpioToChannel(pin);
            REQUIRE(channel.has_value());
            adcStub2.test_setChannelValue(channel->second, 4321);
        }

        const auto readings = presentReadings(discovery.readAdc2());
        REQUIRE(readings.size() == presentPins(GpioDiscovery::getAdc2Pins()).size());
        for (const auto& [pin, value] : readings) {
            (void)pin;
            REQUIRE(value == 4321);
        }
    }

    SECTION("begin accepts a channel already registered by Gate") {
        const auto channel = AdcGpioMapping::gpioToChannel(GPIO_NUM_1);
        REQUIRE(channel.has_value());
        REQUIRE(adcStub.registerChannel(channel->second) == ESP_OK);
        adcStub.test_setChannelValue(channel->second, 777);

        discovery.begin();

        const auto readings = presentReadings(discovery.readAdc1());
        const auto it = std::find_if(readings.begin(), readings.end(), [](const auto& r) { return r.first == GPIO_NUM_1; });
        REQUIRE(it != readings.end());
        REQUIRE(it->second == 777);
    }

    SECTION("getPinLevel is nullopt before begin, and for reserved/adc1/adc2/unknown pins") {
        REQUIRE_FALSE(discovery.getPinLevel(GPIO_NUM_21).has_value());

        discovery.begin();

        REQUIRE_FALSE(discovery.getPinLevel(GPIO_NUM_0).has_value());  // reserved
        REQUIRE_FALSE(discovery.getPinLevel(GPIO_NUM_1).has_value());  // adc1
        REQUIRE_FALSE(discovery.getPinLevel(GPIO_NUM_15).has_value()); // adc2
    }

    SECTION("begin drives a pin bound elsewhere as it is") {
        REQUIRE(gpioStub.gpioSetDirection(GPIO_NUM_48, GPIO_MODE_OUTPUT) == ESP_OK);
        REQUIRE(gpioStub.gpioSetLevel(GPIO_NUM_48, 1) == ESP_OK);
        REQUIRE(pinRegister.bindPin(GPIO_NUM_48));

        discovery.begin();

        REQUIRE(gpioStub.test_gpioGetMode(GPIO_NUM_48) == GPIO_MODE_OUTPUT);
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_48) == 0);
        REQUIRE(discovery.setPinLevel(GPIO_NUM_48, true));
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_48) == 1);
    }

    SECTION("end drives a borrowed pin low and keeps it an output, while resetting the pins it configured") {
        REQUIRE(gpioStub.gpioSetDirection(GPIO_NUM_48, GPIO_MODE_OUTPUT) == ESP_OK);
        REQUIRE(pinRegister.bindPin(GPIO_NUM_48));
        discovery.begin();
        REQUIRE(discovery.setPinLevel(GPIO_NUM_48, true));

        discovery.end();

        REQUIRE(gpioStub.test_gpioGetMode(GPIO_NUM_48) == GPIO_MODE_OUTPUT);
        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_48) == 0);
        REQUIRE(gpioStub.test_gpioGetMode(GPIO_NUM_21) == GPIO_MODE_DISABLE);
    }

    SECTION("end without begin resets no pins") {
        REQUIRE(gpioStub.gpioSetDirection(GPIO_NUM_48, GPIO_MODE_OUTPUT) == ESP_OK);

        discovery.end();

        REQUIRE(gpioStub.test_gpioGetMode(GPIO_NUM_48) == GPIO_MODE_OUTPUT);
    }

    SECTION("getDrivenPins lists every free output pin, bound or not") {
        REQUIRE(pinRegister.bindPin(GPIO_NUM_48));

        discovery.begin();

        REQUIRE(presentPins(discovery.getDrivenPins()) == presentPins(GpioDiscovery::getFreeOutputPins()));
    }

    SECTION("end clears tracked output levels") {
        discovery.begin();
        REQUIRE(discovery.setPinLevel(GPIO_NUM_21, true));

        discovery.end();

        REQUIRE_FALSE(discovery.getPinLevel(GPIO_NUM_21).has_value());
    }
}

TEST_CASE("GpioDiscovery: setPinLevel", "[GpioDiscovery]") {
    GpioStub gpioStub{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    AdcOneshotStub adcStub2(ADC_UNIT_2);
    GpioPinRegister pinRegister{};
    GpioDiscovery discovery(pinRegister, gpioStub, adcStub, adcStub2);
    discovery.begin();

    SECTION("drives a free output pin and updates getPinLevel") {
        REQUIRE(discovery.setPinLevel(GPIO_NUM_21, true));

        REQUIRE(gpioStub.test_gpioGetLevel(GPIO_NUM_21) == 1);
        REQUIRE(discovery.getPinLevel(GPIO_NUM_21) == std::optional(true));
    }

    SECTION("rejects a reserved pin") {
        REQUIRE_FALSE(discovery.setPinLevel(GPIO_NUM_0, true));
    }

    SECTION("rejects an adc1 pin") {
        REQUIRE_FALSE(discovery.setPinLevel(GPIO_NUM_1, true));
    }

    SECTION("rejects an adc2 pin") {
        REQUIRE_FALSE(discovery.setPinLevel(GPIO_NUM_15, true));
    }

    SECTION("rejects pins before begin()") {
        GpioDiscovery freshDiscovery(pinRegister, gpioStub, adcStub, adcStub2);
        REQUIRE_FALSE(freshDiscovery.setPinLevel(GPIO_NUM_21, true));
    }
}
