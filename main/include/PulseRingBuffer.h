#ifndef LASERGATE_TESTS_PULSERINGBUFFER_H
#define LASERGATE_TESTS_PULSERINGBUFFER_H

#include <cstdint>

/**
 * Tracks a rolling window of the last 32 laser pulse verification results in a ring buffer,
 * where each bit records whether that pulse matched its expected state.
 */
class PulseRingBuffer {
public:
    PulseRingBuffer() noexcept = default;

    /**
     * Records the outcome of the most recent pulse verification, overwriting the oldest entry.
     */
    void insertResult(bool pulseMatched) noexcept;

    /**
     * Clears all recorded results back to the initial (all-matched) state.
     */
    void reset() noexcept;

    /**
     * @return The number of mismatched pulses currently recorded in the ring buffer.
     * Not meaningful until getSampleCount() reaches 32.
     */
    [[nodiscard]] uint8_t getFailureCount() const noexcept;

    /**
     * @return The number of matched pulses currently recorded in the ring buffer.
     * Not meaningful until getSampleCount() reaches 32.
     */
    [[nodiscard]] uint8_t getSuccessCount() const noexcept;

    /**
     * @return The number of real results recorded since the last reset, capped at 32.
     */
    [[nodiscard]] uint8_t getSampleCount() const noexcept;

private:
    // Each bit records one pulse result; a cleared bit marks a mismatch.
    uint32_t ringBuffer = 0xFFFFFFFF;
    uint8_t ringBufferPointer = 0;
    uint8_t sampleCount = 0;
};


#endif //LASERGATE_TESTS_PULSERINGBUFFER_H
