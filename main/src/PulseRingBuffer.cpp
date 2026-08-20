#include "PulseRingBuffer.h"

#include <bit>

void PulseRingBuffer::insertResult(bool sensorDetectedOn, bool laserActuallyOn) noexcept {
    const uint32_t mask = 1u << ringBufferPointer;

    sensedBuffer = sensorDetectedOn ? (sensedBuffer | mask) : (sensedBuffer & ~mask);
    expectedBuffer = laserActuallyOn ? (expectedBuffer | mask) : (expectedBuffer & ~mask);

    ++ringBufferPointer;
    if (ringBufferPointer >= static_cast<uint8_t>(sizeof(sensedBuffer) * 8)) {
        ringBufferPointer = 0;
    }
    if (sampleCount < 32) {
        ++sampleCount;
    }
}

void PulseRingBuffer::reset() noexcept {
    sensedBuffer = 0xFFFFFFFF;
    expectedBuffer = 0xFFFFFFFF;
    ringBufferPointer = 0;
    sampleCount = 0;
}

uint8_t PulseRingBuffer::getFailureCount() const noexcept {
    return static_cast<uint8_t>(std::popcount(sensedBuffer ^ expectedBuffer));
}

uint8_t PulseRingBuffer::getSuccessCount() const noexcept {
    return static_cast<uint8_t>(std::popcount(~(sensedBuffer ^ expectedBuffer)));
}

uint8_t PulseRingBuffer::getFalsePositiveCount() const noexcept {
    return static_cast<uint8_t>(std::popcount(sensedBuffer & ~expectedBuffer));
}

uint8_t PulseRingBuffer::getFalseNegativeCount() const noexcept {
    return static_cast<uint8_t>(std::popcount(~sensedBuffer & expectedBuffer));
}

uint8_t PulseRingBuffer::getSampleCount() const noexcept {
    return sampleCount;
}
