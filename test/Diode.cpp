#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <vector>
#include "../main/include/Diode.h"
#include "../main/include/LaserDiode.h"
#include "../main/include/LightEmittingDiode.h"
#include "../main/include/GpioPinRegister.h"
#include "../main/include/platform/GpioDigitalWritePin.h"
#include "test/stubs/GpioStub.h"

TEMPLATE_TEST_CASE("Diode: polymorphic dispatch", "[Diode]", LaserDiode, LightEmittingDiode) {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    GpioDigitalWritePin gpioPin{pr, gpioStub, GPIO_NUM_19};
    TestType concrete{gpioPin};
    Diode& diode = concrete;

    SECTION("initializes through the base class reference") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.isReady());
    }

    SECTION("turns the underlying gpio pin on through the base class reference") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.turnOn());
        REQUIRE(gpioPin.getState() == PIN_STATE_DIGITAL::HIGH);
    }

    SECTION("turns the underlying gpio pin off through the base class reference") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.turnOn());
        REQUIRE(diode.turnOff());
        REQUIRE(gpioPin.getState() == PIN_STATE_DIGITAL::LOW);
    }

    SECTION("frees through the base class reference") {
        REQUIRE(diode.initialize());
        REQUIRE(diode.free());
        REQUIRE_FALSE(diode.isReady());
    }
}

TEST_CASE("Diode: heterogeneous collection of concrete diodes", "[Diode]") {
    GpioPinRegister pr{};
    GpioStub gpioStub{};
    GpioDigitalWritePin laserPin{pr, gpioStub, GPIO_NUM_18};
    GpioDigitalWritePin ledPin{pr, gpioStub, GPIO_NUM_19};

    LaserDiode laser{laserPin};
    LightEmittingDiode led{ledPin};

    std::vector<std::reference_wrapper<Diode>> diodes{laser, led};

    for (Diode& diode : diodes) {
        REQUIRE(diode.initialize());
        REQUIRE(diode.setPowerState(true));
    }

    REQUIRE(laserPin.getState() == PIN_STATE_DIGITAL::HIGH);
    REQUIRE(ledPin.getState() == PIN_STATE_DIGITAL::HIGH);
}
