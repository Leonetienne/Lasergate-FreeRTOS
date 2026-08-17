//
// Created by Leon Etienne on 18.03.26.
//

#ifndef LASERGATE_V2_STATES_H
#define LASERGATE_V2_STATES_H

#include <cstdint>

enum class STATE : uint8_t {
    INITIALIZATION, // System is starting up
    OPERATIONAL,    // System is fine and operating normally
    FAULT,          // The system has entered an invalid state.
    SHUTTING_DOWN   // System is shutting down
};

#endif //LASERGATE_V2_STATES_H
