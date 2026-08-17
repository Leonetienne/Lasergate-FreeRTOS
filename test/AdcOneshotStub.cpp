//
// Created by Leon Etienne on 3/22/26.
//

#include <catch2/catch_test_macros.hpp>
#include "test/stubs/AdcOneshotStub.h"

TEST_CASE("AdcOneshotStub", "[AdcOneshotStub]") {
    AdcOneshotStub stubAdc1(ADC_UNIT_1);

    SECTION("not ready by default") {
        REQUIRE_FALSE(stubAdc1.isReady());
    }

    SECTION("ready after initialize") {
        REQUIRE(stubAdc1.initialize() == ESP_OK);
        REQUIRE(stubAdc1.isReady());
    }

    SECTION("not ready after move") {
        AdcOneshotStub stub2 = std::move(stubAdc1);
        REQUIRE_FALSE(stubAdc1.isReady());
    }

    SECTION("moved-to is ready after move if it was initialized") {
        REQUIRE(stubAdc1.initialize() == ESP_OK);
        AdcOneshotStub stub2 = std::move(stubAdc1);
        REQUIRE_FALSE(stubAdc1.isReady());
        REQUIRE(stub2.isReady());
    }

    SECTION("reading uninitialized channel fails") {
        const auto readResult = stubAdc1.readChannel(ADC_CHANNEL_6);
        REQUIRE_FALSE(readResult.has_value());
    }

    SECTION("can initialize channel") {
        REQUIRE(stubAdc1.registerChannel(ADC_CHANNEL_6) == ESP_OK);
    }

    SECTION("can't initialize off-unit channel on unit 1") {
        REQUIRE_FALSE(stubAdc1.registerChannel(ADC_CHANNEL_9) == ESP_OK);
    }

    SECTION("can initialize adc-2 unit channel on adc2 adc driver") {
        AdcOneshotStub stubAdc2(ADC_UNIT_2);
        REQUIRE(stubAdc2.registerChannel(ADC_CHANNEL_9) == ESP_OK);
    }

    SECTION("can't initialize off-unit channel on unit 2") {
        AdcOneshotStub stubAdc2(ADC_UNIT_2);
        REQUIRE_FALSE(stubAdc2.registerChannel(ADC_CHANNEL_6) == ESP_OK);
    }

    SECTION("reading initialized channel works and returns 0") {
        REQUIRE(stubAdc1.registerChannel(ADC_CHANNEL_6) == ESP_OK);
        const auto readResult = stubAdc1.readChannel(ADC_CHANNEL_6);
        REQUIRE(readResult.has_value());
        REQUIRE(readResult == 0);
    }

    SECTION("reading initialized channel works and returns stored value") {
        REQUIRE(stubAdc1.registerChannel(ADC_CHANNEL_6) == ESP_OK);
        stubAdc1.test_setChannelValue(ADC_CHANNEL_6, 2484);
        const auto readResult = stubAdc1.readChannel(ADC_CHANNEL_6);
        REQUIRE(readResult.has_value());
        REQUIRE(readResult == 2484);
    }
}
