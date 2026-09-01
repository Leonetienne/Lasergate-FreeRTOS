#ifndef LASERGATE_V2_GATEECONFIG_H
#define LASERGATE_V2_GATEECONFIG_H

#include <cstdint>

// Tunables for Gate

// If more than n gatemodules are interrupted at any time, raise alarm state
// If less than n gatemodules are interrupted for at least x ms, raise alarm
// x is the time the slowest pulsing gate module needs to complete a batch.
constexpr uint8_t ALLOWED_SHORT_TERM_INTERRUPTED_GATES = 1;

#endif //LASERGATE_V2_GATEECONFIG_H
