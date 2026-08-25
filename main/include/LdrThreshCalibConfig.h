#ifndef LASERGATE_V2_LDRTHRESHCALIBCONFIG_H
#define LASERGATE_V2_LDRTHRESHCALIBCONFIG_H

#include <cstdint>

// Tunables for GateModule's CALIBRATION_LDR_THRESH routine.

constexpr int       CALIB_LDR_TRESH_PULSE_FREQ      = 500; // ms
constexpr uint16_t  CALIB_LDR_THRESH_INITIAL_THRESH = 3000; // MUST satisfy CALIB_LDR_TRESH_MIN_THRESH << CALIB_LDR_THRESH_INITIAL_THRESH << CALIB_LDR_TRESH_MAX_THRESH
constexpr uint16_t  CALIB_LDR_TRESH_MIN_THRESH      = 500; // lower limit of plausible values
constexpr uint16_t  CALIB_LDR_TRESH_MAX_THRESH      = 4095; // 12-bit ADC ceiling
constexpr float     CALIB_LDR_TRESH_STEP_FACTOR     = 0.2f; // change by 20% each iteration
constexpr float     CALIB_LDR_TRESH_TARGET_IN_RANGE = 0.75f; // calibration target is position 75% of valid range
constexpr uint16_t  CALIB_LDR_TRESH_MIN_STEP_SIZE   = 50; // prevent super slow or stuck calibration

#endif //LASERGATE_V2_LDRTHRESHCALIBCONFIG_H
