//
// Created by nixmage on 3/22/26.
//

#include <catch2/catch_test_macros.hpp>
#include "test/stubs/AdcOneshotStub.h"

TEST_CASE("AdcOneshotStub", "[AdcOneshotStub]") {
    AdcOneshotStub stub(ADC_UNIT_1);

    SECTION("ready by default") {
        REQUIRE(stub.isReady());
    }

    SECTION("not ready after move") {
        AdcOneshotStub stub2 = std::move(stub);
        REQUIRE_FALSE(stub.isReady());
    }

    SECTION("moved-to is ready after move") {
        AdcOneshotStub stub2 = std::move(stub);
        REQUIRE_FALSE(stub.isReady());
        REQUIRE(stub2.isReady());
    }


    SECTION("reading uninitialized channel fails") {
        int buf;
        REQUIRE_FALSE(stub.readChannel(ADC_CHANNEL_6, buf) == ESP_OK);
    }

    SECTION("reading initialized channel works and returns 0") {
        REQUIRE(stub.registerChannel(ADC_CHANNEL_6) == ESP_OK);
        int buf;
        REQUIRE(stub.readChannel(ADC_CHANNEL_6, buf) == ESP_OK);
        REQUIRE(buf == 0);
    }

    SECTION("reading initialized channel works and returns stored value") {
        REQUIRE(stub.registerChannel(ADC_CHANNEL_6) == ESP_OK);
        int buf;
        stub.test_setChannelValue(ADC_CHANNEL_6, 2484);
        REQUIRE(stub.readChannel(ADC_CHANNEL_6, buf) == ESP_OK);
        REQUIRE(buf == 2484);
    }
}
