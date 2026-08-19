#include "../include/GateModule.h"

constexpr int INITIAL_LDR_THRESH = 0;

GateModule::GateModule(
    LaserDiode &laserDiode,
    LightEmittingDiode &statusLed,
    LaserSensor &laserSensor
) noexcept:
    laserDiode {laserDiode},
    statusLed {statusLed},
    laserSensor {laserSensor}
{ }

GateModule::~GateModule() noexcept {
    if (isInitialized) {
        free();
    }
}

bool GateModule::initialize() noexcept {
    if (isInitialized) {
        return false;
    }

    bool success = true;

    if (!laserDiode.initialize()) {
        success = false;
    }
    if (!statusLed.initialize()) {
        success = false;
    }
    if (!laserSensor.initialize(INITIAL_LDR_THRESH)) {
        success = false;
    }

    if (success) {
        isInitialized = true;
    }

    return success;
}

bool GateModule::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    bool success = true;

    if (!laserDiode.free()) {
        success = false;
    }
    if (!statusLed.free()) {
        success = false;
    }
    if (!laserSensor.free()) {
        success = false;
    }

    isInitialized = false;

    return success;
}

bool GateModule::isReady() const noexcept {
    return isInitialized;
}

void GateModule::fixedUpdate() {
}
