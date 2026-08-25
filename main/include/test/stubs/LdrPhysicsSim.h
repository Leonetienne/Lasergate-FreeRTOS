#ifndef LASERGATE_V2_LDRPHYSICSSIM_H
#define LASERGATE_V2_LDRPHYSICSSIM_H

#include <cstdint>

/**
 * Simulates the physical response of an LDR to a laser being switched on/off: the raw
 * reading ramps linearly between an ambient and a lit level over a fixed rise/fall time,
 * instead of jumping instantaneously.
 */
class LdrPhysicsSim {
public:
    LdrPhysicsSim(uint16_t ambient, uint16_t lit, int64_t rampDurationMillis) noexcept;

    /**
     * Informs the simulation that the laser's actual power state changed.
     * Interrupting an in-progress ramp starts a new one from the current (partial) reading.
     */
    void setPowerState(bool desiredPowerState, int64_t nowMillis) noexcept;

    /**
     * @return The simulated raw LDR reading at the given time
     */
    [[nodiscard]] uint16_t getCurrentReading(int64_t nowMillis) const noexcept;

private:
    uint16_t ambientReading;
    uint16_t litReading;
    int64_t rampMillis;

    bool laserOn = false;
    int64_t transitionStartMillis = 0;
    uint16_t rampStartReading;
};

#endif //LASERGATE_V2_LDRPHYSICSSIM_H
