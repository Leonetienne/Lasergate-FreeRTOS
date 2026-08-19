#ifndef LASERGATE_TESTS_GATEMODULE_H
#define LASERGATE_TESTS_GATEMODULE_H

#include "LaserDiode.h"
#include "LightEmittingDiode.h"
#include "LaserSensor.h"

/**
 * Aggregates and drives a status led, a laser diode and a laser sensor into a module
 * that is able to detect objects interrupting the laser
 */
class GateModule {
public:
    GateModule(LaserDiode& laserDiode, LightEmittingDiode& statusLed, LaserSensor& laserSensor) noexcept;
    GateModule(const GateModule&) = delete;
    GateModule(GateModule&&) = delete;
    GateModule& operator=(const GateModule&) = delete;
    GateModule& operator=(GateModule&&) = delete;
    ~GateModule() noexcept;

    /**
    * Will initialize this module
    * @return Success state
    */
    bool initialize() noexcept;

    /**
     * Will free all resources acquired by this module
     * @return Success state
     */
    bool free() noexcept;

    /**
     * @return Whether this moduls is ready for operation
     */
    [[nodiscard]] bool isReady() const noexcept;

    /**
     * Call every 10ms
     */
    void fixedUpdate();

private:
    bool isInitialized = false;

    LaserDiode& laserDiode;
    LightEmittingDiode& statusLed;
    LaserSensor& laserSensor;
};


#endif //LASERGATE_TESTS_GATEMODULE_H
