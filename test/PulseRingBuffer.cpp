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

    SECTION("no false positives recorded") {
        REQUIRE(buffer.getFalsePositiveCount() == 0);
    }

    SECTION("no false negatives recorded") {
        REQUIRE(buffer.getFalseNegativeCount() == 0);
    }

    SECTION("no samples recorded") {
        REQUIRE(buffer.getSampleCount() == 0);
    }
}

TEST_CASE("PulseRingBuffer: insertResult", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    SECTION("a matched 'on' pulse records no failure") {
        buffer.insertResult(true, true);
        REQUIRE(buffer.getFailureCount() == 0);
        REQUIRE(buffer.getSuccessCount() == 32);
        REQUIRE(buffer.getSampleCount() == 1);
    }

    SECTION("a matched 'off' pulse records no failure") {
        buffer.insertResult(false, false);
        REQUIRE(buffer.getFailureCount() == 0);
        REQUIRE(buffer.getSuccessCount() == 32);
        REQUIRE(buffer.getSampleCount() == 1);
    }

    SECTION("sensor detected on, but laser was actually off, records a false positive") {
        buffer.insertResult(true, false);
        REQUIRE(buffer.getFailureCount() == 1);
        REQUIRE(buffer.getSuccessCount() == 31);
        REQUIRE(buffer.getFalsePositiveCount() == 1);
        REQUIRE(buffer.getFalseNegativeCount() == 0);
        REQUIRE(buffer.getSampleCount() == 1);
    }

    SECTION("sensor detected off, but laser was actually on, records a false negative") {
        buffer.insertResult(false, true);
        REQUIRE(buffer.getFailureCount() == 1);
        REQUIRE(buffer.getSuccessCount() == 31);
        REQUIRE(buffer.getFalsePositiveCount() == 0);
        REQUIRE(buffer.getFalseNegativeCount() == 1);
        REQUIRE(buffer.getSampleCount() == 1);
    }

    SECTION("multiple mismatches accumulate by kind") {
        buffer.insertResult(true, false);  // false positive
        buffer.insertResult(true, true);   // match
        buffer.insertResult(false, true);  // false negative
        buffer.insertResult(true, false);  // false positive

        REQUIRE(buffer.getFailureCount() == 3);
        REQUIRE(buffer.getSuccessCount() == 29);
        REQUIRE(buffer.getFalsePositiveCount() == 2);
        REQUIRE(buffer.getFalseNegativeCount() == 1);
        REQUIRE(buffer.getSampleCount() == 4);
    }

    SECTION("sample count caps at 32") {
        for (int i = 0; i < 40; ++i) {
            buffer.insertResult(true, true);
        }

        REQUIRE(buffer.getSampleCount() == 32);
    }

    SECTION("wrapping around the ring buffer overwrites the oldest entries") {
        // Fill all 32 slots with false positives
        for (int i = 0; i < 32; ++i) {
            buffer.insertResult(true, false);
        }
        REQUIRE(buffer.getFailureCount() == 32);
        REQUIRE(buffer.getSuccessCount() == 0);
        REQUIRE(buffer.getFalsePositiveCount() == 32);
        REQUIRE(buffer.getFalseNegativeCount() == 0);

        // Overwrite the first 5 slots with matches
        for (int i = 0; i < 5; ++i) {
            buffer.insertResult(true, true);
        }

        REQUIRE(buffer.getFailureCount() == 27);
        REQUIRE(buffer.getSuccessCount() == 5);
        REQUIRE(buffer.getFalsePositiveCount() == 27);
        REQUIRE(buffer.getFalseNegativeCount() == 0);
        REQUIRE(buffer.getSampleCount() == 32);
    }
}

TEST_CASE("PulseRingBuffer: reset", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    for (int i = 0; i < 6; ++i) {
        buffer.insertResult(true, false); // false positive
    }
    for (int i = 0; i < 4; ++i) {
        buffer.insertResult(false, true); // false negative
    }
    REQUIRE(buffer.getFailureCount() == 10);
    REQUIRE(buffer.getSuccessCount() == 22);
    REQUIRE(buffer.getFalsePositiveCount() == 6);
    REQUIRE(buffer.getFalseNegativeCount() == 4);
    REQUIRE(buffer.getSampleCount() == 10);

    buffer.reset();

    SECTION("clears recorded failures") {
        REQUIRE(buffer.getFailureCount() == 0);
    }

    SECTION("resets recorded successes to full") {
        REQUIRE(buffer.getSuccessCount() == 32);
    }

    SECTION("clears recorded false positives") {
        REQUIRE(buffer.getFalsePositiveCount() == 0);
    }

    SECTION("clears recorded false negatives") {
        REQUIRE(buffer.getFalseNegativeCount() == 0);
    }

    SECTION("clears the sample count") {
        REQUIRE(buffer.getSampleCount() == 0);
    }

    SECTION("accepts new results after resetting") {
        buffer.insertResult(true, false);
        REQUIRE(buffer.getFailureCount() == 1);
        REQUIRE(buffer.getFalsePositiveCount() == 1);
        REQUIRE(buffer.getSampleCount() == 1);
    }
}
