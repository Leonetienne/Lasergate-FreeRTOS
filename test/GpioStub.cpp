//
// Created by Leon Etienne on 21.03.26.
//

#include <catch2/catch_test_macros.hpp>
#include "test/stubs/GpioStub.h"

TEST_CASE("GpioStub: All pins are disabled at start") {
    // Prepare
    GpioStub gpStub;

    // Validate
    for (std::size_t i = 0; i < 64; i++) {
        REQUIRE(gpStub.test_gpioGetMode(i) == GPIO_MODE_DISABLE);
    }
}

TEST_CASE("GpioStub: Setting pin direction stores direction") {
    // Prepare
    GpioStub gpStub;
    gpStub.gpioSetDirection(19, GPIO_MODE_OUTPUT);

    // Validate
    REQUIRE(gpStub.test_gpioGetMode(19) == GPIO_MODE_OUTPUT);
}

TEST_CASE("GpioStub: All pins are at level 0 at start") {
    // Prepare
    GpioStub gpStub;

    // Validate
    for (gpio_num_t i = 0; i < 64; i++) {
        REQUIRE(gpStub.test_gpioGetMode(i) == 0);
    }
}

TEST_CASE("GpioStub: Setting a pin level stores level") {
    // Prepare
    GpioStub gpStub;
    for (gpio_num_t i = 0; i < 64; i++) {
        const uint32_t level = i * i;
        gpStub.gpioSetlevel(i, level);
    }

    // Validate
    for (gpio_num_t i = 0; i < 64; i++) {
        const uint32_t level = i * i;
        REQUIRE(gpStub.test_gpioGetLevel(i) == level);
    }
}