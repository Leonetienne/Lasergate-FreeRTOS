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

TEST_CASE("PulseRingBuffer: isSaturated", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    SECTION("false initially, since no samples have been recorded") {
        REQUIRE_FALSE(buffer.isSaturated());
    }

    SECTION("stays false while fewer than 32 samples have been recorded") {
        for (int i = 0; i < 31; ++i) {
            buffer.insertResult(true, true);
        }
        REQUIRE(buffer.getSampleCount() == 31);
        REQUIRE_FALSE(buffer.isSaturated());
    }

    SECTION("becomes true once 32 samples have been recorded") {
        for (int i = 0; i < 32; ++i) {
            buffer.insertResult(true, true);
        }
        REQUIRE(buffer.getSampleCount() == 32);
        REQUIRE(buffer.isSaturated());
    }

    SECTION("stays true once the sample count caps at 32, regardless of mismatches") {
        for (int i = 0; i < 40; ++i) {
            buffer.insertResult(true, false);
        }
        REQUIRE(buffer.getSampleCount() == 32);
        REQUIRE(buffer.isSaturated());
    }
}

TEST_CASE("PulseRingBuffer: at", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    SECTION("true initially, since every default slot counts as a match") {
        REQUIRE(buffer.at(0));
        REQUIRE(buffer.at(31));
    }

    SECTION("reflects the most recently inserted matching result") {
        buffer.insertResult(true, true);
        REQUIRE(buffer.at(0));
    }

    SECTION("reflects the most recently inserted mismatched result") {
        buffer.insertResult(true, false);
        REQUIRE_FALSE(buffer.at(0));
    }

    SECTION("untouched slots still report the default matched state") {
        buffer.insertResult(true, true);
        REQUIRE(buffer.at(5));
    }

    SECTION("older results shift to higher indices as new ones are inserted") {
        buffer.insertResult(true, false);  // becomes the second-most-recent result
        buffer.insertResult(true, true);   // most recent result

        REQUIRE(buffer.at(0));
        REQUIRE_FALSE(buffer.at(1));
    }

    SECTION("index wraps around modulo 32") {
        buffer.insertResult(true, false); // written to slot 0
        for (int i = 0; i < 31; ++i) {
            buffer.insertResult(true, true); // fills the remaining 31 slots
        }

        REQUIRE_FALSE(buffer.at(31));
        REQUIRE_FALSE(buffer.at(31 + 32));
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
