#ifndef LASERGATE_V2_RANDOMSTUB_H
#define LASERGATE_V2_RANDOMSTUB_H

#include "hal/IRandom.h"
#include <random>

class RandomStub : public IRandom {
public:
    RandomStub() noexcept;
    RandomStub(const RandomStub&) = delete;
    RandomStub(RandomStub&&) = delete;

    /**
     * @return A freshly generated random 32-bit integer, sourced from a mt19937 engine
     */
    [[nodiscard]] int32_t getNextInt() noexcept override;

    /**
     * Testing stub method: seed the underlying engine for deterministic output
     */
    void test_setSeed(uint32_t seed) noexcept;

private:
    std::mt19937 engine;
};

#endif //LASERGATE_V2_RANDOMSTUB_H
