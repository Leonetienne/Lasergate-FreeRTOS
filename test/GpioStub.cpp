//
// Created by Leon Etienne on 21.03.26.
//

#include <catch2/catch_test_macros.hpp>
#include "test/stubs/GpioStub.h"

TEST_CASE("GpioStub", "[GpioStub]") {
    constexpr gpio_num_t testPin = GPIO_NUM_19;
    GpioStub gpStub;

    SECTION("all pins are disabled at start") {
        for (std::size_t pin = 0; pin < 64; ++pin) {
            CHECK(gpStub.test_gpioGetMode(static_cast<gpio_num_t>(pin)) == GPIO_MODE_DISABLE);
        }
    }

    SECTION("setting pin direction stores direction") {
        REQUIRE(gpStub.gpioSetDirection(testPin, GPIO_MODE_OUTPUT) == ESP_OK);
        REQUIRE(gpStub.test_gpioGetMode(testPin) == GPIO_MODE_OUTPUT);
    }

    SECTION("setting pin direction to input stores direction") {
        REQUIRE(gpStub.gpioSetDirection(testPin, GPIO_MODE_INPUT) == ESP_OK);
        REQUIRE(gpStub.test_gpioGetMode(testPin) == GPIO_MODE_INPUT);
    }

    SECTION("setting pin direction to anything but input/output fails") {
        REQUIRE(gpStub.gpioSetDirection(testPin, GPIO_MODE_OUTPUT_OD) == ESP_ERR_NOT_SUPPORTED);
    }

    SECTION("an input pin defaults to level 1 (idle/released) until explicitly set") {
        REQUIRE(gpStub.gpioGetLevel(testPin) == 1);
    }

    SECTION("test_setInputLevel controls what gpioGetLevel reads back") {
        gpStub.test_setInputLevel(testPin, 0);
        REQUIRE(gpStub.gpioGetLevel(testPin) == 0);

        gpStub.test_setInputLevel(testPin, 1);
        REQUIRE(gpStub.gpioGetLevel(testPin) == 1);
    }

    SECTION("all pins are at level 0 at start") {
        for (std::size_t pin = 0; pin < 64; ++pin) {
            CHECK(gpStub.test_gpioGetLevel(static_cast<gpio_num_t>(pin)) == 0);
        }
    }

    SECTION("setting a pin level stores level") {
        for (std::size_t pin = 0; pin < 64; ++pin) {
            const uint32_t level = pin * pin;
            REQUIRE(gpStub.gpioSetLevel(static_cast<gpio_num_t>(pin), level) == ESP_OK);
        }

        for (std::size_t pin = 0; pin < 64; ++pin) {
            const uint32_t level = pin * pin;
            CHECK(gpStub.test_gpioGetLevel(static_cast<gpio_num_t>(pin)) == level);
        }
    }
}