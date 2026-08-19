#include "PulseRingBuffer.h"

#include <bit>

void PulseRingBuffer::insertResult(bool pulseMatched) noexcept {
    if (pulseMatched) {
        ringBuffer |= (1u << ringBufferPointer);
    } else {
        ringBuffer &= ~(1u << ringBufferPointer);
    }
    ++ringBufferPointer;
    if (ringBufferPointer >= static_cast<uint8_t>(sizeof(ringBuffer) * 8)) {
        ringBufferPointer = 0;
    }
    if (sampleCount < 32) {
        ++sampleCount;
    }
}

void PulseRingBuffer::reset() noexcept {
    ringBuffer = 0xFFFFFFFF;
    ringBufferPointer = 0;
    sampleCount = 0;
}

uint8_t PulseRingBuffer::getFailureCount() const noexcept {
    return static_cast<uint8_t>(std::popcount(static_cast<uint32_t>(~ringBuffer)));
}

uint8_t PulseRingBuffer::getSuccessCount() const noexcept {
    return static_cast<uint8_t>(std::popcount(static_cast<uint32_t>(ringBuffer)));
}

uint8_t PulseRingBuffer::getSampleCount() const noexcept {
    return sampleCount;
}
