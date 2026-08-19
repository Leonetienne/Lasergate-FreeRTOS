#include "hal/IRandom.h"

bool IRandom::getNextBit() noexcept {
    if (bitCursor >= 32) {
        bitBuffer = getNextInt();
        bitCursor = 0;
    }

    const bool bit = (bitBuffer >> bitCursor) & 1;
    ++bitCursor;

    return bit;
}
