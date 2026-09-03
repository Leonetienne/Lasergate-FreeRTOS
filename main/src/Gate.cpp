#include "Gate.h"
#include "GateConfig.h"
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
    },
    i_time {i_time}
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
    // Update gatemodule calibrators if in calibration state
    if (stateMachine.getState() == STATE::CALIBRATION_LDR_THRESH) {
        bool allDone = true;
        for (std::size_t i = 0; i < modules.size(); ++i) {
            if (!modules[i].isReady()) continue; // unconfigured modules don't block completion
            ldrCalibrators[i].fixedUpdate();

            if (ldrCalibrators[i].status() == LdrThreshCalibrator::Status::FAILED) {
                stateMachine.setState(STATE::FAULT, "Gate: LDR threshold calibration failed for module " + std::to_string(i) + ": " + ldrCalibrators[i].failureReason());
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
                stateMachine.setState(STATE::FAULT,"Gate: laser pulse frequency calibration failed for module " + std::to_string(i) + ": " + freqCalibrators[i].failureReason());
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

    // These only run in non-calibration states

    // Update gatemodule fixedUpdate hook
    for (GateModule& module : modules) {
        if (module.isReady()) {
            module.fixedUpdate();
        }
    }

    if (stateMachine.getState() == STATE::OBSERVING) {
        // Update alarm invariants
        // Count how many gates are interrupted
        uint8_t numInterruptedGateModules = 0;
        for (GateModule& module : modules) {
            // We default to pulse batch acceptance to true because we need the warmup phase
            // to not raise any interruptions, as during the warmup phase readings from the pulse ring puffer are meaningless.
            // read errors already raise either fault or misreads which are both forwarded to clients (either as alarm or fault).
            if (module.isReady() && !module.isPulseBatchAcceptable().value_or(true)) {
                numInterruptedGateModules++;
            }
        }

        // Reset alarm invariant if no modules are interrupted
        if (numInterruptedGateModules == 0) {
            resetLenientAlarmState();
        }
        // Handle lenient state: numInterruptedGateModules <= ALLOWED_SHORT_TERM_INTERRUPTED_GATES
        else if (numInterruptedGateModules <= ALLOWED_SHORT_TERM_INTERRUPTED_GATES) {
            // Initialize lenient alarm state if it is not already
            if (!lenientAlarmActive) {
                startLenientAlarmState();
            }
            // If it is, raise actual alarm state, if the lenient phase persisted long enough
            else if (i_time.getMillis() - lenientAlarmTimeout >= longestBatchTime) {
                resetLenientAlarmState();
                stateMachine.setState(STATE::ALARM);
            }
        }
        // Raise alarm state immediately if numInterruptedGateModules > ALLOWED_SHORT_TERM_DISRUPTED_GATES
        else if (numInterruptedGateModules > ALLOWED_SHORT_TERM_INTERRUPTED_GATES) {
            resetLenientAlarmState();
            stateMachine.setState(STATE::ALARM);
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
        case STATE::OBSERVING:
            resetLenientAlarmState();
            recalculateLongestBatchTime();
            break;
        default:
            break;
    }

    for (GateModule& module : modules) {
        if (module.isReady()) {
            module.onStateChange();
        }
    }
}

void Gate::startLenientAlarmState() noexcept {
    lenientAlarmActive = true;
    lenientAlarmTimeout = i_time.getMillis();
}

void Gate::resetLenientAlarmState() noexcept {
    lenientAlarmActive = false;
    lenientAlarmTimeout = 0;
}

void Gate::recalculateLongestBatchTime() noexcept {
    longestBatchTime = 0;
    for (GateModule& module : modules) {
        if (module.isReady()) {
            const auto batchTime = module.getBatchTime();
            if (batchTime.has_value()) {
                if (*batchTime > longestBatchTime) {
                    longestBatchTime = *batchTime;
                }
            }
        }
    }

    // Fallback value.
    if (longestBatchTime == 0) {
        longestBatchTime = LEANIENT_ALARM_DELAY_FALLBACK_MS - LEANIENT_ALARM_DELAY_LEEWAY_MS;
    }

    // Add 100ms leeway, to make sure the old batch did definitely clear
    longestBatchTime += LEANIENT_ALARM_DELAY_LEEWAY_MS;
}
