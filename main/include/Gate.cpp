#include "Gate.h"

Gate::Gate(StateMachine &stateMachine, std::array<GateModule, 1> gateModules) noexcept:
    stateMachine {stateMachine},
    modules {gateModules}
{ }

Gate::~Gate() noexcept {
    if (isInitialized) {
        free();
    }
}

bool Gate::initialize() noexcept {
    if (isInitialized) {
        return false;
    }

    bool success = true;

    for (GateModule& module : modules) {
        if (!module.initialize()) {
            success = false;
            stateMachine.setLastFaultReason("gate module initialization failed");
        }
    }

    if (success) {
        isInitialized = true;
    }

    return success;
}

bool Gate::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    bool success = true;

    for (GateModule& module : modules) {
        if (!module.free()) {
            success = false;
            stateMachine.setLastFaultReason("gate module freeing failed");
        }
    }

    isInitialized = false;

    return success;
}

bool Gate::isReady() const noexcept {
    return isInitialized;
}

void Gate::fixedUpdate() noexcept {
    for (GateModule& module : modules) {
        module.fixedUpdate();
    }
}

void Gate::onStateChange() noexcept {
    for (GateModule& module : modules) {
        module.onStateChange();
    }
}
