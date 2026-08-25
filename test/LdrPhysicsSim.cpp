#include <catch2/catch_test_macros.hpp>
#include "test/stubs/LdrPhysicsSim.h"

TEST_CASE("LdrPhysicsSim: resting state", "[LdrPhysicsSim]") {
    LdrPhysicsSim sim(800, 9000, 150);

    SECTION("starts at the ambient reading") {
        REQUIRE(sim.getCurrentReading(0) == 800);
    }

    SECTION("stays at the ambient reading indefinitely without a transition") {
        REQUIRE(sim.getCurrentReading(1'000'000) == 800);
    }
}

TEST_CASE("LdrPhysicsSim: ramping on", "[LdrPhysicsSim]") {
    LdrPhysicsSim sim(800, 9000, 150);
    sim.setPowerState(true, 1000);

    SECTION("reading is unchanged at the instant the laser turns on") {
        REQUIRE(sim.getCurrentReading(1000) == 800);
    }

    SECTION("reading is partway between ambient and lit mid-ramp") {
        const uint16_t reading = sim.getCurrentReading(1075); // halfway through the 150ms ramp
        REQUIRE(reading > 800);
        REQUIRE(reading < 9000);
    }

    SECTION("reading reaches the lit level once the ramp elapses") {
        REQUIRE(sim.getCurrentReading(1150) == 9000);
    }

    SECTION("reading stays at the lit level long after the ramp elapses") {
        REQUIRE(sim.getCurrentReading(100'000) == 9000);
    }
}

TEST_CASE("LdrPhysicsSim: ramping off", "[LdrPhysicsSim]") {
    LdrPhysicsSim sim(800, 9000, 150);
    sim.setPowerState(true, 0);
    sim.setPowerState(false, 1000); // laser was already fully lit by now

    SECTION("reading is unchanged at the instant the laser turns off") {
        REQUIRE(sim.getCurrentReading(1000) == 9000);
    }

    SECTION("reading is partway between lit and ambient mid-ramp") {
        const uint16_t reading = sim.getCurrentReading(1075);
        REQUIRE(reading > 800);
        REQUIRE(reading < 9000);
    }

    SECTION("reading settles back to ambient once the ramp elapses") {
        REQUIRE(sim.getCurrentReading(1150) == 800);
    }
}

TEST_CASE("LdrPhysicsSim: settles well before a 500ms pulse period", "[LdrPhysicsSim]") {
    LdrPhysicsSim sim(800, 9000, 150);
    sim.setPowerState(true, 0);

    REQUIRE(sim.getCurrentReading(500) == 9000);
}

TEST_CASE("LdrPhysicsSim: interrupting a ramp starts a new one from the current position", "[LdrPhysicsSim]") {
    LdrPhysicsSim sim(800, 9000, 150);
    sim.setPowerState(true, 0);

    const uint16_t partialReading = sim.getCurrentReading(50); // 1/3 into the rise
    REQUIRE(partialReading > 800);
    REQUIRE(partialReading < 9000);

    sim.setPowerState(false, 50);

    // The descent must continue from wherever the rise had gotten to, not from the full lit level
    REQUIRE(sim.getCurrentReading(60) <= partialReading);
    REQUIRE(sim.getCurrentReading(125) < partialReading);
    REQUIRE(sim.getCurrentReading(125) > 800);
    REQUIRE(sim.getCurrentReading(200) == 800); // fully settled 150ms after the interrupt
}
