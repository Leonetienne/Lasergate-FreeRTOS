#include <catch2/catch_test_macros.hpp>
#include "hal/BoardReservedPins.h"
#include <algorithm>

TEST_CASE("BoardReservedPins: ALL_PINS", "[BoardReservedPins]") {
    SECTION("lists every esp32-s3 gpio once, ascending, gpio22-25 excluded") {
        REQUIRE(BoardReservedPins::ALL_PINS.size() == 45);
        REQUIRE(std::is_sorted(BoardReservedPins::ALL_PINS.begin(), BoardReservedPins::ALL_PINS.end()));
        REQUIRE(std::adjacent_find(BoardReservedPins::ALL_PINS.begin(), BoardReservedPins::ALL_PINS.end()) == BoardReservedPins::ALL_PINS.end());
        for (int pin = 22; pin <= 25; ++pin) {
            REQUIRE(std::find(BoardReservedPins::ALL_PINS.begin(), BoardReservedPins::ALL_PINS.end(), static_cast<gpio_num_t>(pin)) == BoardReservedPins::ALL_PINS.end());
        }
        REQUIRE(BoardReservedPins::ALL_PINS.front() == GPIO_NUM_0);
        REQUIRE(BoardReservedPins::ALL_PINS.back() == GPIO_NUM_48);
    }
}

TEST_CASE("BoardReservedPins: isReserved", "[BoardReservedPins]") {
    SECTION("flags strapping pins") {
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_0));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_3));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_45));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_46));
    }

    SECTION("flags the in-package spi flash range") {
        for (int pin = GPIO_NUM_26; pin <= GPIO_NUM_32; ++pin) {
            REQUIRE(BoardReservedPins::isReserved(static_cast<gpio_num_t>(pin)));
        }
    }

    SECTION("flags gpio33-37 as occupied on this module") {
        for (int pin = GPIO_NUM_33; pin <= GPIO_NUM_37; ++pin) {
            REQUIRE(BoardReservedPins::isReserved(static_cast<gpio_num_t>(pin)));
        }
    }

    SECTION("flags the native usb pins") {
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_19));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_20));
    }

    SECTION("flags the uart0 console pins") {
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_43));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_44));
    }

    SECTION("flags the onboard w5500 spi bus, interrupt and reset pins") {
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_9));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_10));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_11));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_12));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_13));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_14));
    }

    SECTION("flags gpio4-8 as not broken out to either header") {
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_4));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_5));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_6));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_7));
        REQUIRE(BoardReservedPins::isReserved(GPIO_NUM_8));
    }

    SECTION("leaves adc1 and other free header pins unreserved") {
        REQUIRE_FALSE(BoardReservedPins::isReserved(GPIO_NUM_1));
        REQUIRE_FALSE(BoardReservedPins::isReserved(GPIO_NUM_2));
        REQUIRE_FALSE(BoardReservedPins::isReserved(GPIO_NUM_15));
        REQUIRE_FALSE(BoardReservedPins::isReserved(GPIO_NUM_40));
        REQUIRE_FALSE(BoardReservedPins::isReserved(GPIO_NUM_48));
    }
}

TEST_CASE("BoardReservedPins: isInputOnly", "[BoardReservedPins]") {
    SECTION("every esp32-s3 pin has an output driver") {
        for (const gpio_num_t pin : BoardReservedPins::ALL_PINS) {
            REQUIRE_FALSE(BoardReservedPins::isInputOnly(pin));
        }
    }
}
