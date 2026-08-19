#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <vector>
#include "../main/include/Diode.h"
#include "../main/include/LaserDiode.h"
#include "../main/include/LightEmittingDiode.h"
#include "../main/include/GpioPinRegister.h"
#include "test/stubs/GpioStub.h"

TEMPLATE_TEST_CASE("Diode: polymorphic dispatch", "[Diode]", LaserDiode, LightEmittingDiode) {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    constexpr gpio_num_t bindPin = GPIO_NUM_19;
    TestType concrete{pr, gpioStub, bindPin};
    Diode& diode = concrete;

    SECTION("initializes through the base class reference") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.isReady());
    }

    SECTION("turns the underlying gpio pin on through the base class reference") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.turnOn());
        REQUIRE(gpioStub.test_gpioGetLevel(bindPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    }

    SECTION("turns the underlying gpio pin off through the base class reference") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.turnOn());
        REQUIRE(diode.turnOff());
        REQUIRE(gpioStub.test_gpioGetLevel(bindPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::LOW));
    }

    SECTION("frees through the base class reference") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.free());
        REQUIRE_FALSE(diode.isReady());
    }
}

TEMPLATE_TEST_CASE("Diode: isConfigured", "[Diode]", LaserDiode, LightEmittingDiode) {
    GpioPinRegister pr{};
    GpioStub gpioStub{};

    SECTION("false when bound to GPIO_NUM_NC") {
        TestType diode{pr, gpioStub, GPIO_NUM_NC};
        REQUIRE_FALSE(diode.isConfigured());
    }

    SECTION("true when bound to a real pin") {
        TestType diode{pr, gpioStub, GPIO_NUM_19};
        REQUIRE(diode.isConfigured());
    }
}

TEST_CASE("Diode: heterogeneous collection of concrete diodes", "[Diode]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    constexpr gpio_num_t laserPin = GPIO_NUM_18;
    constexpr gpio_num_t ledPin = GPIO_NUM_19;

    LaserDiode laser{pr, gpioStub, laserPin};
    LightEmittingDiode led{pr, gpioStub, ledPin};

    std::vector<std::reference_wrapper<Diode>> diodes{laser, led};

    for (Diode& diode : diodes) {
        REQUIRE(diode.initialize());
        REQUIRE(diode.setPowerState(true));
    }

    REQUIRE(gpioStub.test_gpioGetLevel(laserPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
    REQUIRE(gpioStub.test_gpioGetLevel(ledPin) == static_cast<uint32_t>(PIN_STATE_DIGITAL::HIGH));
}
