//
// Created by Leon Etienne on 21.03.26.
//

#include <catch2/catch_test_macros.hpp>
#include "GpioPinRegister.h"

TEST_CASE("GpioPinRegister: All pins are free at start") {
    // Prepare
    GpioPinRegister pr;

    // Validate
    for (gpio_num_t i = 0; i < 64; i++) {
        REQUIRE_FALSE(pr.isPinBound(i));
    }
}

TEST_CASE("GpioPinRegister: Can bind unbound pin") {
    // Prepare
    GpioPinRegister pr;

    // Validate
    REQUIRE(pr.bindPin(16));
}

TEST_CASE("GpioPinRegister: Pin is used after being assigned") {
    // Prepare
    GpioPinRegister pr;
    pr.bindPin(16);

    // Validate
    REQUIRE(pr.isPinBound(16));
}

TEST_CASE("GpioPinRegister: Other pins remain free after assigning a pin") {
    // Prepare
    GpioPinRegister pr;
    pr.bindPin(16);

    // Validate
    for (gpio_num_t i = 0; i < 64; i++) {
        if (i != 16) {
            REQUIRE_FALSE(pr.isPinBound(i));
        }
    }
}

TEST_CASE("GpioPinRegister: Can't free unbound pin") {
    // Prepare
    GpioPinRegister pr;

    // Validate
    REQUIRE_FALSE(pr.freePin(16));
}

TEST_CASE("GpioPinRegister: Can free bound pin") {
    // Prepare
    GpioPinRegister pr;
    pr.bindPin(16);

    // Validate
    REQUIRE(pr.freePin(16));
}

TEST_CASE("GpioPinRegister: Can't bind pin twice") {
    // Prepare
    GpioPinRegister pr;
    pr.bindPin(16);

    // Validate
    REQUIRE_FALSE(pr.bindPin(16));
}

TEST_CASE("GpioPinRegister: Can bind pin twice if it is freed first") {
    // Prepare
    GpioPinRegister pr;
    pr.bindPin(16);
    pr.freePin(16);

    // Validate
    REQUIRE(pr.bindPin(16));
}
