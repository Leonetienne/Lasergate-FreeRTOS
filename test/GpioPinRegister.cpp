//
// Created by Leon Etienne on 21.03.26.
//

#include <catch2/catch_test_macros.hpp>
#include "GpioPinRegister.h"

TEST_CASE("GpioPinRegister", "[GpioPinRegister]") {
    constexpr gpio_num_t testPin = 16;
    GpioPinRegister pr;

    SECTION("all pins are free at start") {
        for (gpio_num_t pin = 0; pin < 64; ++pin) {
            CHECK_FALSE(pr.isPinBound(pin));
        }
    }

    SECTION("can bind an unbound pin") {
        REQUIRE(pr.bindPin(testPin));
    }

    SECTION("pin is used after being assigned") {
        REQUIRE(pr.bindPin(testPin));
        REQUIRE(pr.isPinBound(testPin));
    }

    SECTION("other pins remain free after assigning a pin") {
        REQUIRE(pr.bindPin(testPin));

        for (gpio_num_t pin = 0; pin < 64; ++pin) {
            if (pin != testPin) {
                CHECK_FALSE(pr.isPinBound(pin));
            }
        }
    }

    SECTION("can't free unbound pin") {
        REQUIRE_FALSE(pr.freePin(testPin));
    }

    SECTION("can free bound pin") {
        REQUIRE(pr.bindPin(testPin));
        REQUIRE(pr.freePin(testPin));
    }

    SECTION("can't bind pin twice") {
        REQUIRE(pr.bindPin(testPin));
        REQUIRE_FALSE(pr.bindPin(testPin));
    }

    SECTION("can bind pin twice if it is freed first") {
        REQUIRE(pr.bindPin(testPin));
        REQUIRE(pr.freePin(testPin));
        REQUIRE(pr.bindPin(testPin));
    }
}
