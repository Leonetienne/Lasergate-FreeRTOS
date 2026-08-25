#include "test/stubs/LdrPhysicsSim.h"

LdrPhysicsSim::LdrPhysicsSim(int ambient, int lit, int64_t rampDurationMillis) noexcept :
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

int LdrPhysicsSim::getCurrentReading(int64_t nowMillis) const noexcept {
    const int target = laserOn ? litReading : ambientReading;

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
    return rampStartReading + static_cast<int>(static_cast<double>(target - rampStartReading) * t);
}
