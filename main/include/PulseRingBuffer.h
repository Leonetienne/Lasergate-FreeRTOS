#ifndef LASERGATE_TESTS_PULSERINGBUFFER_H
#define LASERGATE_TESTS_PULSERINGBUFFER_H

#include <cstddef>
#include <cstdint>

/**
 * Tracks a rolling window of the last getBufferSize() laser pulse verification results in a ring buffer,
 * recording both what the sensor detected and what the laser was actually set to.
 */
class PulseRingBuffer {
public:
    PulseRingBuffer() noexcept = default;

    /**
     * Records the outcome of the most recent pulse verification, overwriting the oldest entry.
     * @param sensorDetectedOn Whether the sensor detected the laser as on
     * @param laserActuallyOn Whether the laser was actually set to on
     */
    void insertResult(bool sensorDetectedOn, bool laserActuallyOn) noexcept;

    /**
     * Clears all recorded results back to the initial (all-matched) state.
     */
    void reset() noexcept;

    /**
     * @return The number of mismatched pulses currently recorded in the ring buffer.
     * Not meaningful until getSampleCount() reaches getBufferSize().
     */
    [[nodiscard]] uint8_t getFailureCount() const noexcept;

    /**
     * @return The number of matched pulses currently recorded in the ring buffer.
     * Not meaningful until getSampleCount() reaches getBufferSize().
     */
    [[nodiscard]] uint8_t getSuccessCount() const noexcept;

    /**
     * @return The number of pulses where the sensor detected the laser as on, but it was actually off.
     * Not meaningful until getSampleCount() reaches getBufferSize().
     */
    [[nodiscard]] uint8_t getFalsePositiveCount() const noexcept;

    /**
     * @return The number of pulses where the sensor detected the laser as off, but it was actually on.
     * Not meaningful until getSampleCount() reaches getBufferSize().
     */
    [[nodiscard]] uint8_t getFalseNegativeCount() const noexcept;

    /**
     * @return The number of real results recorded since the last reset, capped at getBufferSize().
     */
    [[nodiscard]] uint8_t getSampleCount() const noexcept;

    /**
     * @return Shorthand for getSampleCount() >= getBufferSize()
     */
    [[nodiscard]] bool isSaturated() const noexcept;

    [[nodiscard]] static constexpr std::size_t getBufferSize() noexcept { return sizeof(sensedBuffer) * 8; }

    /**
     * @param index How many pulses back to look, with 0 being the most recently inserted result.
     * Wraps around (modulo getBufferSize()) for values >= getBufferSize().
     * @return True if the sensor reading matched the actual laser state for that pulse, false otherwise.
     */
    [[nodiscard]] bool at(const uint8_t index) const noexcept;

private:
    // Each bit records what the sensor detected for one pulse
    uint32_t sensedBuffer = 0xFFFFFFFF;
    // Each bit records what the laser was actually set to for one pulse
    uint32_t expectedBuffer = 0xFFFFFFFF;
    uint8_t ringBufferPointer = 0;
    uint8_t sampleCount = 0;
};


#endif //LASERGATE_TESTS_PULSERINGBUFFER_H
