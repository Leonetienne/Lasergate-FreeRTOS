#ifndef LASERGATE_V2_SYSTEMESP32_H
#define LASERGATE_V2_SYSTEMESP32_H

#include "System.h"

/**
 * Wires up a System instance against the esp32 platform implementation of every hal interface.
 * @return The esp32-backed System instance
 */
System& getSystem() noexcept;

#endif //LASERGATE_V2_SYSTEMESP32_H
