#include <catch2/catch_test_macros.hpp>
#include "../main/include/PulseRingBuffer.h"

TEST_CASE("PulseRingBuffer: initial state", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    SECTION("no failures recorded") {
        REQUIRE(buffer.getFailureCount() == 0);
    }

    SECTION("all slots count as successes") {
        REQUIRE(buffer.getSuccessCount() == 32);
    }

    SECTION("no samples recorded") {
        REQUIRE(buffer.getSampleCount() == 0);
    }
}

TEST_CASE("PulseRingBuffer: insertResult", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    SECTION("a single matched pulse records no failure") {
        buffer.insertResult(true);
        REQUIRE(buffer.getFailureCount() == 0);
        REQUIRE(buffer.getSuccessCount() == 32);
        REQUIRE(buffer.getSampleCount() == 1);
    }

    SECTION("a single mismatched pulse records one failure") {
        buffer.insertResult(false);
        REQUIRE(buffer.getFailureCount() == 1);
        REQUIRE(buffer.getSuccessCount() == 31);
        REQUIRE(buffer.getSampleCount() == 1);
    }

    SECTION("multiple mismatches accumulate") {
        buffer.insertResult(false);
        buffer.insertResult(true);
        buffer.insertResult(false);
        buffer.insertResult(false);

        REQUIRE(buffer.getFailureCount() == 3);
        REQUIRE(buffer.getSuccessCount() == 29);
        REQUIRE(buffer.getSampleCount() == 4);
    }

    SECTION("sample count caps at 32") {
        for (int i = 0; i < 40; ++i) {
            buffer.insertResult(true);
        }

        REQUIRE(buffer.getSampleCount() == 32);
    }

    SECTION("wrapping around the ring buffer overwrites the oldest entries") {
        // Fill all 32 slots with failures
        for (int i = 0; i < 32; ++i) {
            buffer.insertResult(false);
        }
        REQUIRE(buffer.getFailureCount() == 32);
        REQUIRE(buffer.getSuccessCount() == 0);

        // Overwrite the first 5 slots with matches
        for (int i = 0; i < 5; ++i) {
            buffer.insertResult(true);
        }

        REQUIRE(buffer.getFailureCount() == 27);
        REQUIRE(buffer.getSuccessCount() == 5);
        REQUIRE(buffer.getSampleCount() == 32);
    }
}

TEST_CASE("PulseRingBuffer: reset", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    for (int i = 0; i < 10; ++i) {
        buffer.insertResult(false);
    }
    REQUIRE(buffer.getFailureCount() == 10);
    REQUIRE(buffer.getSuccessCount() == 22);
    REQUIRE(buffer.getSampleCount() == 10);

    buffer.reset();

    SECTION("clears recorded failures") {
        REQUIRE(buffer.getFailureCount() == 0);
    }

    SECTION("resets recorded successes to full") {
        REQUIRE(buffer.getSuccessCount() == 32);
    }

    SECTION("clears the sample count") {
        REQUIRE(buffer.getSampleCount() == 0);
    }

    SECTION("accepts new results after resetting") {
        buffer.insertResult(false);
        REQUIRE(buffer.getFailureCount() == 1);
        REQUIRE(buffer.getSampleCount() == 1);
    }
}
