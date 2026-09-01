#include <catch2/catch_test_macros.hpp>
#include "../main/include/PulseRingBuffer.h"

TEST_CASE("PulseRingBuffer: initial state", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    SECTION("no failures recorded") {
        REQUIRE(buffer.getFailureCount() == 0);
    }

    SECTION("all slots count as successes") {
        REQUIRE(buffer.getSuccessCount() == PulseRingBuffer::getBufferSize());
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
        REQUIRE(buffer.getSuccessCount() == PulseRingBuffer::getBufferSize());
        REQUIRE(buffer.getSampleCount() == 1);
    }

    SECTION("a matched 'off' pulse records no failure") {
        buffer.insertResult(false, false);
        REQUIRE(buffer.getFailureCount() == 0);
        REQUIRE(buffer.getSuccessCount() == PulseRingBuffer::getBufferSize());
        REQUIRE(buffer.getSampleCount() == 1);
    }

    SECTION("sensor detected on, but laser was actually off, records a false positive") {
        buffer.insertResult(true, false);
        REQUIRE(buffer.getFailureCount() == 1);
        REQUIRE(buffer.getSuccessCount() == PulseRingBuffer::getBufferSize() - 1);
        REQUIRE(buffer.getFalsePositiveCount() == 1);
        REQUIRE(buffer.getFalseNegativeCount() == 0);
        REQUIRE(buffer.getSampleCount() == 1);
    }

    SECTION("sensor detected off, but laser was actually on, records a false negative") {
        buffer.insertResult(false, true);
        REQUIRE(buffer.getFailureCount() == 1);
        REQUIRE(buffer.getSuccessCount() == PulseRingBuffer::getBufferSize() - 1);
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
        REQUIRE(buffer.getSuccessCount() == PulseRingBuffer::getBufferSize() - 3);
        REQUIRE(buffer.getFalsePositiveCount() == 2);
        REQUIRE(buffer.getFalseNegativeCount() == 1);
        REQUIRE(buffer.getSampleCount() == 4);
    }

    SECTION("sample count caps at PulseRingBuffer::getBufferSize()") {
        for (std::size_t i = 0; i < PulseRingBuffer::getBufferSize() + 8; ++i) {
            buffer.insertResult(true, true);
        }

        REQUIRE(buffer.getSampleCount() == PulseRingBuffer::getBufferSize());
    }

    SECTION("wrapping around the ring buffer overwrites the oldest entries") {
        // Fill all PulseRingBuffer::getBufferSize() slots with false positives
        for (std::size_t i = 0; i < PulseRingBuffer::getBufferSize(); ++i) {
            buffer.insertResult(true, false);
        }
        REQUIRE(buffer.getFailureCount() == PulseRingBuffer::getBufferSize());
        REQUIRE(buffer.getSuccessCount() == 0);
        REQUIRE(buffer.getFalsePositiveCount() == PulseRingBuffer::getBufferSize());
        REQUIRE(buffer.getFalseNegativeCount() == 0);

        // Overwrite the first 5 slots with matches
        for (std::size_t i = 0; i < 5; ++i) {
            buffer.insertResult(true, true);
        }

        REQUIRE(buffer.getFailureCount() == PulseRingBuffer::getBufferSize() - 5);
        REQUIRE(buffer.getSuccessCount() == 5);
        REQUIRE(buffer.getFalsePositiveCount() == PulseRingBuffer::getBufferSize() - 5);
        REQUIRE(buffer.getFalseNegativeCount() == 0);
        REQUIRE(buffer.getSampleCount() == PulseRingBuffer::getBufferSize());
    }
}

TEST_CASE("PulseRingBuffer: isSaturated", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    SECTION("false initially, since no samples have been recorded") {
        REQUIRE_FALSE(buffer.isSaturated());
    }

    SECTION("stays false while fewer than PulseRingBuffer::getBufferSize() samples have been recorded") {
        for (std::size_t i = 0; i < PulseRingBuffer::getBufferSize() - 1; ++i) {
            buffer.insertResult(true, true);
        }
        REQUIRE(buffer.getSampleCount() == PulseRingBuffer::getBufferSize() - 1);
        REQUIRE_FALSE(buffer.isSaturated());
    }

    SECTION("becomes true once PulseRingBuffer::getBufferSize() samples have been recorded") {
        for (std::size_t i = 0; i < PulseRingBuffer::getBufferSize(); ++i) {
            buffer.insertResult(true, true);
        }
        REQUIRE(buffer.getSampleCount() == PulseRingBuffer::getBufferSize());
        REQUIRE(buffer.isSaturated());
    }

    SECTION("stays true once the sample count caps at PulseRingBuffer::getBufferSize(), regardless of mismatches") {
        for (std::size_t i = 0; i < PulseRingBuffer::getBufferSize() + 8; ++i) {
            buffer.insertResult(true, false);
        }
        REQUIRE(buffer.getSampleCount() == PulseRingBuffer::getBufferSize());
        REQUIRE(buffer.isSaturated());
    }
}

TEST_CASE("PulseRingBuffer: at", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    SECTION("true initially, since every default slot counts as a match") {
        REQUIRE(buffer.at(0));
        REQUIRE(buffer.at(PulseRingBuffer::getBufferSize() - 1));
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

    SECTION("index wraps around modulo PulseRingBuffer::getBufferSize()") {
        buffer.insertResult(true, false); // written to slot 0
        for (std::size_t i = 0; i < PulseRingBuffer::getBufferSize() - 1; ++i) {
            buffer.insertResult(true, true); // fills the remaining slots
        }

        REQUIRE_FALSE(buffer.at(PulseRingBuffer::getBufferSize() - 1));
        REQUIRE_FALSE(buffer.at((PulseRingBuffer::getBufferSize() - 1) + PulseRingBuffer::getBufferSize()));
    }
}

TEST_CASE("PulseRingBuffer: reset", "[PulseRingBuffer]") {
    PulseRingBuffer buffer;

    const std::size_t falsePositives = PulseRingBuffer::getBufferSize() / 4;
    const std::size_t falseNegatives = PulseRingBuffer::getBufferSize() / 8;

    for (std::size_t i = 0; i < falsePositives; ++i) {
        buffer.insertResult(true, false); // false positive
    }
    for (std::size_t i = 0; i < falseNegatives; ++i) {
        buffer.insertResult(false, true); // false negative
    }
    REQUIRE(buffer.getFailureCount() == falsePositives + falseNegatives);
    REQUIRE(buffer.getSuccessCount() == PulseRingBuffer::getBufferSize() - (falsePositives + falseNegatives));
    REQUIRE(buffer.getFalsePositiveCount() == falsePositives);
    REQUIRE(buffer.getFalseNegativeCount() == falseNegatives);
    REQUIRE(buffer.getSampleCount() == falsePositives + falseNegatives);

    buffer.reset();

    SECTION("clears recorded failures") {
        REQUIRE(buffer.getFailureCount() == 0);
    }

    SECTION("resets recorded successes to full") {
        REQUIRE(buffer.getSuccessCount() == PulseRingBuffer::getBufferSize());
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
