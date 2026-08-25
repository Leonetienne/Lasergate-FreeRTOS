#ifndef LASERGATE_V2_LASERPULSEFREQCALIBCONFIG_H
#define LASERGATE_V2_LASERPULSEFREQCALIBCONFIG_H

#include <cstdint>

// Tunables for GateModule's CALIBRATION_LDR_THRESH routine.

constexpr uint16_t  CALIB_PULSE_FREQ_MIN_FREQ       = 50; // ms
constexpr uint16_t  CALIB_PULSE_FREQ_MAX_FREQ       = 800; // ms
constexpr float     CALIB_PULSE_FREQ_STEP_FACTOR    = 0.2f; // change by 20% each iteration
constexpr uint16_t  CALIB_PULSE_FREQ_MIN_STEP_SIZE  = 50; // prevent super slow or stuck calibration

#endif //LASERGATE_V2_LASERPULSEFREQCALIBCONFIG_H
