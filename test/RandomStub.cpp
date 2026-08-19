#include <catch2/catch_test_macros.hpp>
#include "test/stubs/RandomStub.h"
#include <array>

TEST_CASE("RandomStub", "[RandomStub]") {
    RandomStub randomStub;

    SECTION("getNextInt returns varying values") {
        // Will falsely fail 0.00000002% of the time
        const int32_t a = randomStub.getNextInt();
        const int32_t b = randomStub.getNextInt();
        REQUIRE(a != b);
    }

    SECTION("getNextBit reproduces getNextInt bit-for-bit, LSB first") {
        randomStub.test_setSeed(999);

        RandomStub reference;
        reference.test_setSeed(999);
        const int32_t expectedInt = reference.getNextInt();

        for (int bitIndex = 0; bitIndex < 32; ++bitIndex) {
            const bool expectedBit = (expectedInt >> bitIndex) & 1;
            REQUIRE(randomStub.getNextBit() == expectedBit);
        }
    }

    SECTION("buffer renews after 32 bits and resumes at bit 0 of the next getNextInt") {
        randomStub.test_setSeed(42);

        RandomStub reference;
        reference.test_setSeed(42);
        reference.getNextInt();
        const int32_t secondInt = reference.getNextInt();

        for (int i = 0; i < 32; ++i) {
            randomStub.getNextBit();
        }

        REQUIRE(randomStub.getNextBit() == static_cast<bool>(secondInt & 1));
    }

    SECTION("getNextBit does not degenerate into a constant sequence (mishandled bitshift regression)") {
        randomStub.test_setSeed(1337);

        bool sawZero = false;
        bool sawOne = false;
        int oneCount = 0;
        constexpr int sampleSize = 10000;

        for (int i = 0; i < sampleSize; ++i) {
            if (randomStub.getNextBit()) {
                sawOne = true;
                ++oneCount;
            } else {
                sawZero = true;
            }
        }

        REQUIRE(sawZero);
        REQUIRE(sawOne);
        // Loose statistical bound: catches a stuck-bit/shift bug without being flaky on genuine randomness.
        REQUIRE(oneCount > sampleSize * 0.3);
        REQUIRE(oneCount < sampleSize * 0.7);
    }

    SECTION("every bit position is observed high at least once across many refills (catches a stuck-cursor/fixed-shift bug)") {
        randomStub.test_setSeed(2024);

        std::array<bool, 32> seenHigh{};
        constexpr int refills = 200;

        for (int r = 0; r < refills; ++r) {
            for (int bitIndex = 0; bitIndex < 32; ++bitIndex) {
                if (randomStub.getNextBit()) {
                    seenHigh[static_cast<size_t>(bitIndex)] = true;
                }
            }
        }

        for (int bitIndex = 0; bitIndex < 32; ++bitIndex) {
            INFO("bitIndex=" << bitIndex);
            REQUIRE(seenHigh[static_cast<size_t>(bitIndex)]);
        }
    }
}
