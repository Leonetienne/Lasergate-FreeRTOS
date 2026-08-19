#ifndef LASERGATE_V2_LIGHTEMITTINGDIODE_H
#define LASERGATE_V2_LIGHTEMITTINGDIODE_H

#include "Diode.h"

/**
 * A gpio-driven light emitting diode
 */
class LightEmittingDiode : public Diode {
public:
    using Diode::Diode;
};


#endif //LASERGATE_V2_LIGHTEMITTINGDIODE_H
