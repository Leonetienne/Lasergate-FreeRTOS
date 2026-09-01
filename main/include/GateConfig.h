#ifndef LASERGATE_V2_GATEECONFIG_H
#define LASERGATE_V2_GATEECONFIG_H

#include <cstdint>

// Tunables for Gate

// If more than n gatemodules are interrupted at any time, raise alarm state
// If less than n gatemodules are interrupted for at least x ms, raise alarm
// x is the time the slowest pulsing gate module needs to complete a batch.
constexpr uint8_t ALLOWED_SHORT_TERM_INTERRUPTED_GATES = 1;
// 5000ms which is pretty conservative
constexpr uint8_t LEANIENT_ALARM_DELAY_FALLBACK_MS = 5000;
// Add these many ms to the leanient alarm timeout as a safety margin
constexpr uint8_t LEANIENT_ALARM_DELAY_LEEWAY_MS = 100;

#endif //LASERGATE_V2_GATEECONFIG_H
