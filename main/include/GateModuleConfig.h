#ifndef LASERGATE_V2_GATEMODULECONFIG_H
#define LASERGATE_V2_GATEMODULECONFIG_H

#include <cstdint>

// Tunables for GateModule

constexpr uint8_t ALLOWED_MISREADS_PER_BATCH = 1; // misreads tolerated in a 32-pulse batch before it's rejected

#endif //LASERGATE_V2_GATEMODULECONFIG_H
