//
// Created by Leon Etienne on 21.03.26.
//

#include <catch2/catch_test_macros.hpp>
#include "GpioPinRegister.h"
#include "platform/AdcAnalogReadPin.h"
#include "test/stubs/AdcOneshotStub.h"

TEST_CASE("AdcAnalogReadPin: lifecycle", "[AdcAnalogReadPin]") {
    GpioPinRegister pr{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    constexpr gpio_num_t bindPin = GPIO_NUM_34;
    AdcAnalogReadPin pin(pr, adcStub, bindPin);

    SECTION("not ready by default") {
        REQUIRE_FALSE(pin.isReady());
    }

    SECTION("can initialize") {
        REQUIRE(pin.initialize() == ESP_OK);
    }

    SECTION("initialize fails with off-unit channel") {
        AdcOneshotStub adcStub2(ADC_UNIT_2);
        AdcAnalogReadPin pin2(pr, adcStub2, bindPin);
        REQUIRE(pin2.initialize() == ESP_ERR_INVALID_ARG);
    }

    SECTION("can't initialize twice") {
        REQUIRE(pin.initialize() == ESP_OK);
        REQUIRE(pin.initialize() == ESP_ERR_INVALID_STATE);
    }

    SECTION("is ready after initialization") {
        REQUIRE(pin.initialize() == ESP_OK);
        REQUIRE(pin.isReady());
    }

    SECTION("pin is bound after initialization") {
        REQUIRE(pin.initialize() == ESP_OK);
        REQUIRE(pr.isPinBound(bindPin));
    }


    SECTION("initialize fails if pin is already bound") {
        REQUIRE(pr.bindPin(bindPin));
        REQUIRE(pin.initialize() == ESP_ERR_INVALID_STATE);
    }

    SECTION("read fails if pin is uninitialized") {
        auto result = pin.read();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == ESP_ERR_INVALID_STATE);
    }

    SECTION("pin can read after initialization") {
        REQUIRE(adcStub.initialize() == ESP_OK);
        REQUIRE(pin.initialize() == ESP_OK);
        auto result = pin.read();
        REQUIRE(result.has_value());
    }

    SECTION("pin reads 0 after initialization") {
        REQUIRE(adcStub.initialize() == ESP_OK);
        REQUIRE(pin.initialize() == ESP_OK);
        auto result = pin.read();
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 0);
    }

    SECTION("pin reads actual value") {
        REQUIRE(adcStub.initialize() == ESP_OK);
        REQUIRE(pin.initialize() == ESP_OK);
        adcStub.test_setChannelValue(ADC_CHANNEL_6, 2734);
        auto result = pin.read();
        REQUIRE(result.value() == 2734);
    }

    SECTION("pin can be freed") {
        REQUIRE(pin.initialize() == ESP_OK);
        REQUIRE(pin.free());
    }

    SECTION("pin is not ready after freeing") {
        REQUIRE(pin.initialize() == ESP_OK);
        REQUIRE(pin.free());
        REQUIRE_FALSE(pin.isReady());
    }

    SECTION("pin can't be freed again") {
        REQUIRE(pin.initialize() == ESP_OK);
        REQUIRE(pin.free());
        REQUIRE_FALSE(pin.free());
    }

    SECTION("free fails when pin is not initialized") {
        REQUIRE_FALSE(pin.free());
    }

    SECTION("pin is no longer bound after freeing") {
        REQUIRE(pin.initialize() == ESP_OK);
        REQUIRE(pin.free());
        REQUIRE_FALSE(pr.isPinBound(bindPin));
    }

    SECTION("moved pin transfers readiness") {
        REQUIRE(pin.initialize() == ESP_OK);

        AdcAnalogReadPin moved{std::move(pin)};

        REQUIRE(moved.isReady());
        REQUIRE_FALSE(pin.isReady());
    }
}
