//
// Created by Leon Etienne on 21.03.26.
//

#include <catch2/catch_test_macros.hpp>
#include "test/stubs/GpioStub.h"

TEST_CASE("GpioStub", "[GpioStub]") {
    constexpr gpio_num_t testPin = 19;
    GpioStub gpStub;

    SECTION("all pins are disabled at start") {
        for (gpio_num_t pin = 0; pin < 64; ++pin) {
            CHECK(gpStub.test_gpioGetMode(pin) == GPIO_MODE_DISABLE);
        }
    }

    SECTION("setting pin direction stores direction") {
        REQUIRE(gpStub.gpioSetDirection(testPin, GPIO_MODE_OUTPUT) == ESP_OK);
        REQUIRE(gpStub.test_gpioGetMode(testPin) == GPIO_MODE_OUTPUT);
    }

    SECTION("setting pin direction to anything but output fails") {
        REQUIRE(gpStub.gpioSetDirection(testPin, GPIO_MODE_INPUT) == ESP_ERR_NOT_SUPPORTED);
    }

    SECTION("all pins are at level 0 at start") {
        for (gpio_num_t pin = 0; pin < 64; ++pin) {
            CHECK(gpStub.test_gpioGetLevel(pin) == 0);
        }
    }

    SECTION("setting a pin level stores level") {
        for (gpio_num_t pin = 0; pin < 64; ++pin) {
            const uint32_t level = pin * pin;
            REQUIRE(gpStub.gpioSetLevel(pin, level) == ESP_OK);
        }

        for (gpio_num_t pin = 0; pin < 64; ++pin) {
            const uint32_t level = pin * pin;
            CHECK(gpStub.test_gpioGetLevel(pin) == level);
        }
    }
}