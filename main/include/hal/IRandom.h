#ifndef LASERGATE_V2_IRANDOM_H
#define LASERGATE_V2_IRANDOM_H

#include <cstdint>

/**
 * Abstract interface for random number generation
 */
class IRandom {
public:
    IRandom() = default;
    IRandom(const IRandom&) = delete;
    IRandom(IRandom&&) = delete;
    virtual ~IRandom() = default;

    /**
     * Consumes one bit from an internal 32-bit buffer, which is renewed via getNextInt() (with the cursor reset to bit 0)
     * once all 32 bits of it have been consumed.
     * @return The next random bit
     */
    [[nodiscard]] bool getNextBit() noexcept;

    /**
     * @return A freshly generated random 32-bit integer
     */
    [[nodiscard]] virtual int32_t getNextInt() noexcept = 0;

private:
    int32_t bitBuffer = 0;
    int bitCursor = 32;
};

#endif //LASERGATE_V2_IRANDOM_H
