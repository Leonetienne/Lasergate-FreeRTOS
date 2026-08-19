#include "../include/Gate.h"

Gate::Gate(
    StateMachine& stateMachine,
    SettingsManager& settings,
    GpioPinRegister& gpioPinRegister,
    IGpio& gpio,
    IAdcOneshot& adcOneshot
) noexcept:
    stateMachine {stateMachine},
    modules {
        GateModule(
            stateMachine,
            gpioPinRegister,
            gpio,
            adcOneshot,
            settings.retrieveGateModuleLaserGpioPin(0).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLedGpioPin(0).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLdrGpioPin(0).value_or(GPIO_NUM_NC)
        ),
        GateModule(
            stateMachine,
            gpioPinRegister,
            gpio,
            adcOneshot,
            settings.retrieveGateModuleLaserGpioPin(1).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLedGpioPin(1).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLdrGpioPin(1).value_or(GPIO_NUM_NC)
        ),
        GateModule(
            stateMachine,
            gpioPinRegister,
            gpio,
            adcOneshot,
            settings.retrieveGateModuleLaserGpioPin(2).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLedGpioPin(2).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLdrGpioPin(2).value_or(GPIO_NUM_NC)
        ),
        GateModule(
            stateMachine,
            gpioPinRegister,
            gpio,
            adcOneshot,
            settings.retrieveGateModuleLaserGpioPin(3).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLedGpioPin(3).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLdrGpioPin(3).value_or(GPIO_NUM_NC)
        )
    }
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
        if (module.isConfigured() && !module.initialize()) {
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
        if (module.isConfigured() && !module.free()) {
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
        if (module.isReady()) {
            module.fixedUpdate();
        }
    }
}

void Gate::onStateChange() noexcept {
    for (GateModule& module : modules) {
        if (module.isReady()) {
            module.onStateChange();
        }
    }
}
