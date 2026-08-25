#include "../include/Gate.h"
#include <string>

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
    },
    ldrCalibrators {
        LdrThreshCalibrator(modules[0]),
        LdrThreshCalibrator(modules[1]),
        LdrThreshCalibrator(modules[2]),
        LdrThreshCalibrator(modules[3])
    },
    freqCalibrators {
        PulseFreqCalibrator(modules[0]),
        PulseFreqCalibrator(modules[1]),
        PulseFreqCalibrator(modules[2]),
        PulseFreqCalibrator(modules[3])
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
    if (stateMachine.getState() == STATE::CALIBRATION_LDR_THRESH) {
        bool allDone = true;
        for (std::size_t i = 0; i < modules.size(); ++i) {
            if (!modules[i].isReady()) continue; // unconfigured modules don't block completion
            ldrCalibrators[i].fixedUpdate();

            if (ldrCalibrators[i].status() == LdrThreshCalibrator::Status::FAILED) {
                stateMachine.setState(STATE::FAULT,
                    "Gate: LDR threshold calibration failed for module " + std::to_string(i) +
                    ": " + ldrCalibrators[i].failureReason());
                return;
            }
            if (ldrCalibrators[i].status() == LdrThreshCalibrator::Status::RUNNING) {
                allDone = false;
            }
        }
        if (allDone) {
            stateMachine.setState(STATE::DISARMED);
        }
        return;
    }

    if (stateMachine.getState() == STATE::CALIBRATION_MODULATION_FREQUENCY) {
        bool allDone = true;
        for (std::size_t i = 0; i < modules.size(); ++i) {
            if (!modules[i].isReady()) continue; // unconfigured modules don't block completion
            freqCalibrators[i].fixedUpdate();

            if (freqCalibrators[i].status() == PulseFreqCalibrator::Status::FAILED) {
                stateMachine.setState(STATE::FAULT,
                    "Gate: laser pulse frequency calibration failed for module " + std::to_string(i) +
                    ": " + freqCalibrators[i].failureReason());
                return;
            }
            if (freqCalibrators[i].status() == PulseFreqCalibrator::Status::RUNNING) {
                allDone = false;
            }
        }
        if (allDone) {
            stateMachine.setState(STATE::DISARMED);
        }
        return;
    }

    for (GateModule& module : modules) {
        if (module.isReady()) {
            module.fixedUpdate();
        }
    }
}

void Gate::onStateChange() noexcept {
    switch (stateMachine.getState()) {
        case STATE::CALIBRATION_LDR_THRESH:
            for (std::size_t i = 0; i < modules.size(); ++i) {
                if (modules[i].isReady()) {
                    ldrCalibrators[i].begin();
                }
            }
            break;
        case STATE::CALIBRATION_MODULATION_FREQUENCY:
            for (std::size_t i = 0; i < modules.size(); ++i) {
                if (modules[i].isReady()) {
                    freqCalibrators[i].begin();
                }
            }
            break;
        default:
            for (GateModule& module : modules) {
                if (module.isReady()) {
                    module.onStateChange();
                }
            }
            break;
    }
}
