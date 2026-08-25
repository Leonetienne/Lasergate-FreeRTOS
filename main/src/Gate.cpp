#include "../include/Gate.h"

Gate::Gate(
    StateMachine& stateMachine,
    SettingsManager& settings,
    GpioPinRegister& gpioPinRegister,
    IGpio& i_gpio,
    IAdcOneshot& i_adcOneshot,
    IRandom& i_random,
    ITime& i_time
) noexcept:
    stateMachine {stateMachine},
    modules {
        GateModule(
            stateMachine,
            settings,
            0,
            gpioPinRegister,
            i_gpio,
            i_adcOneshot,
            i_random,
            i_time,
            settings.retrieveGateModuleLaserGpioPin(0).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLedGpioPin(0).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLdrGpioPin(0).value_or(GPIO_NUM_NC)
        ),
        GateModule(
            stateMachine,
            settings,
            1,
            gpioPinRegister,
            i_gpio,
            i_adcOneshot,
            i_random,
            i_time,
            settings.retrieveGateModuleLaserGpioPin(1).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLedGpioPin(1).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLdrGpioPin(1).value_or(GPIO_NUM_NC)
        ),
        GateModule(
            stateMachine,
            settings,
            2,
            gpioPinRegister,
            i_gpio,
            i_adcOneshot,
            i_random,
            i_time,
            settings.retrieveGateModuleLaserGpioPin(2).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLedGpioPin(2).value_or(GPIO_NUM_NC),
            settings.retrieveGateModuleLdrGpioPin(2).value_or(GPIO_NUM_NC)
        ),
        GateModule(
            stateMachine,
            settings,
            3,
            gpioPinRegister,
            i_gpio,
            i_adcOneshot,
            i_random,
            i_time,
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
            stateMachine.setState(STATE::FAULT, "Gate::initialize: gate module initialization failed");
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
            stateMachine.setState(STATE::FAULT, "Gate::free: gate module freeing failed");
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
