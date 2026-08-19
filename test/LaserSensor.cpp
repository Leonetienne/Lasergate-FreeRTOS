#include <catch2/catch_test_macros.hpp>
#include "GpioPinRegister.h"
#include "LaserSensor.h"
#include "test/stubs/AdcOneshotStub.h"

TEST_CASE("LaserSensor: lifecycle", "[LaserSensor]") {
    GpioPinRegister pr{};
    AdcOneshotStub adcStub(ADC_UNIT_1);
    constexpr gpio_num_t bindPin = GPIO_NUM_34;
    LaserSensor sensor{pr, adcStub, bindPin};

    REQUIRE(adcStub.initialize() == ESP_OK);

    SECTION("not ready by default") {
        REQUIRE_FALSE(sensor.isReady());
    }

    SECTION("can initialize") {
        REQUIRE(sensor.initialize(500));
    }

    SECTION("can't initialize twice") {
        REQUIRE(sensor.initialize(500));
        REQUIRE_FALSE(sensor.initialize(500));
    }

    SECTION("is ready after initialization") {
        REQUIRE(sensor.initialize(500));
        REQUIRE(sensor.isReady());
    }

    SECTION("initialize fails if underlying ldr pin fails to initialize") {
        REQUIRE(pr.bindPin(bindPin));
        REQUIRE_FALSE(sensor.initialize(500));
    }

    SECTION("stores the threshold passed to initialize") {
        REQUIRE(sensor.initialize(500));
        REQUIRE(sensor.getThreshold() == 500);
    }

    SECTION("threshold can be changed after initialization") {
        REQUIRE(sensor.initialize(500));
        sensor.setThreshold(750);
        REQUIRE(sensor.getThreshold() == 750);
    }

    SECTION("getRawReading fails when not initialized") {
        auto result = sensor.getRawReading();
        REQUIRE_FALSE(result.has_value());
        REQUIRE_FALSE(result.error());
    }

    SECTION("sensesLight fails when not initialized") {
        auto result = sensor.sensesLight();
        REQUIRE_FALSE(result.has_value());
        REQUIRE_FALSE(result.error());
    }

    SECTION("getRawReading returns 0 by default") {
        REQUIRE(sensor.initialize(500));
        auto result = sensor.getRawReading();
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 0);
    }

    SECTION("getRawReading returns the current ldr reading") {
        REQUIRE(sensor.initialize(500));
        adcStub.test_setChannelValue(ADC_CHANNEL_6, 800);
        auto result = sensor.getRawReading();
        REQUIRE(result.value() == 800);
    }

    SECTION("sensesLight is false when reading equals the threshold") {
        REQUIRE(sensor.initialize(500));
        adcStub.test_setChannelValue(ADC_CHANNEL_6, 500);
        auto result = sensor.sensesLight();
        REQUIRE(result.value() == false);
    }

    SECTION("sensesLight is false when reading is below the threshold") {
        REQUIRE(sensor.initialize(500));
        adcStub.test_setChannelValue(ADC_CHANNEL_6, 200);
        auto result = sensor.sensesLight();
        REQUIRE(result.value() == false);
    }

    SECTION("sensesLight is true when reading is above the threshold") {
        REQUIRE(sensor.initialize(500));
        adcStub.test_setChannelValue(ADC_CHANNEL_6, 900);
        auto result = sensor.sensesLight();
        REQUIRE(result.value() == true);
    }

    SECTION("sensesLight reflects a changed threshold") {
        REQUIRE(sensor.initialize(500));
        adcStub.test_setChannelValue(ADC_CHANNEL_6, 600);
        REQUIRE(sensor.sensesLight().value() == true);

        sensor.setThreshold(700);
        REQUIRE(sensor.sensesLight().value() == false);
    }

    SECTION("sensor can be freed") {
        REQUIRE(sensor.initialize(500));
        REQUIRE(sensor.free());
    }

    SECTION("sensor is not ready after freeing") {
        REQUIRE(sensor.initialize(500));
        REQUIRE(sensor.free());
        REQUIRE_FALSE(sensor.isReady());
    }

    SECTION("sensor can't be freed again") {
        REQUIRE(sensor.initialize(500));
        REQUIRE(sensor.free());
        REQUIRE_FALSE(sensor.free());
    }

    SECTION("free fails when not initialized") {
        REQUIRE_FALSE(sensor.free());
    }
}
