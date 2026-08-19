#ifndef LASERGATE_V2_RANDOMESP32_H
#define LASERGATE_V2_RANDOMESP32_H

#include "hal/IRandom.h"

/**
 * Esp32-Implementation for random number generation
 */
class RandomEsp32 : public IRandom {
public:
    RandomEsp32() = default;
    RandomEsp32(const RandomEsp32&) = delete;
    RandomEsp32(RandomEsp32&&) = delete;

    /**
     * @return A freshly generated random 32-bit integer, sourced from esp_random()
     */
    [[nodiscard]] int32_t getNextInt() noexcept override;
};

#endif //LASERGATE_V2_RANDOMESP32_H
