//
// Created by Leon Etienne on 18.03.26.
//

#ifndef LASERGATE_V2_STATES_H
#define LASERGATE_V2_STATES_H

#include <cstdint>

enum class STATE : uint8_t {
    INITIALIZING,           // System is starting up
    USER_ADJUSTING_BEAMS,   // The user is tuning laser beam angle to hit the LDRs. Lasers must fire constantly and status leds must be on if laser is hitting LDR. No pulse modulation and person detection.
    CALIBRATION_LDR_THRESH, // The modules are calibrating their LDR threshold to reliably but greedily detect laser diode power state.
    CALIBRATION_MODULATION_FREQUENCY, // The modules are greedily calibrating the frequency at which they may modulate.
    OBSERVING,      // System is operating normally and watching for gate interruptions.
    DIAGNOSTIC_SIGNAL_TEST_RUN, // The system runs a few batches as a test run to report how noise the laser pulse modulation channel is.
    DISARMED,       // The system is paused, lasers are offline.
    ALARM,          // The system has detected an intrusion
    FAULT,          // The system has entered an invalid state.
    SHUTTING_DOWN   // System is shutting down
};

#endif //LASERGATE_V2_STATES_H
