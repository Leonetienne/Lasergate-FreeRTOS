#include "test/stubs/LdrPhysicsSim.h"

LdrPhysicsSim::LdrPhysicsSim(uint16_t ambient, uint16_t lit, int64_t rampDurationMillis) noexcept :
    ambientReading {ambient},
    litReading {lit},
    rampMillis {rampDurationMillis},
    rampStartReading {ambient}
{ }

void LdrPhysicsSim::setPowerState(bool desiredPowerState, int64_t nowMillis) noexcept {
    if (desiredPowerState == laserOn) {
        return;
    }

    rampStartReading = getCurrentReading(nowMillis);
    laserOn = desiredPowerState;
    transitionStartMillis = nowMillis;
}

uint16_t LdrPhysicsSim::getCurrentReading(int64_t nowMillis) const noexcept {
    const uint16_t target = laserOn ? litReading : ambientReading;

    if (rampMillis <= 0) {
        return target;
    }

    const int64_t elapsed = nowMillis - transitionStartMillis;
    if (elapsed <= 0) {
        return rampStartReading;
    }
    if (elapsed >= rampMillis) {
        return target;
    }

    const double t = static_cast<double>(elapsed) / static_cast<double>(rampMillis);
    return static_cast<uint16_t>(rampStartReading + static_cast<int>(static_cast<double>(target - rampStartReading) * t));
}
