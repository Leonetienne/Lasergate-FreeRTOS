#ifndef LASERGATE_V2_LASERDIODE_H
#define LASERGATE_V2_LASERDIODE_H

#include "Diode.h"

/**
 * A gpio-driven laser diode
 */
class LaserDiode : public Diode {
public:
    using Diode::Diode;
};


#endif //LASERGATE_V2_LASERDIODE_H
