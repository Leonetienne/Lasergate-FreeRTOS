#ifndef LASERGATE_V2_GATEMODULECONFIG_H
#define LASERGATE_V2_GATEMODULECONFIG_H

#include <cstdint>

// Tunables for GateModule

constexpr uint8_t ALLOWED_MISREADS_PER_BATCH = 1; // misreads tolerated in a 32-pulse batch before it's rejected
constexpr uint16_t ALARM_STATE_STATUS_LED_BLINK_INTERVAL = 100; // Toggle the status led every n ms during alarm
constexpr uint16_t FAULT_STATE_STATUS_LED_BLINK_INTERVAL = 500; // Toggle the status led every n ms during fault

#endif //LASERGATE_V2_GATEMODULECONFIG_H
