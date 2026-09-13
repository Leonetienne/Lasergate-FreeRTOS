#include "StateMachine.h"
#include "compat/esp_log_macros.h"
#include <array>

static const char* LOG_TAG = "StateMachine";

StateMachine::StateMachine() noexcept :
    currentState { STATE::INITIALIZING }
{
}

STATE StateMachine::getState() const noexcept {
    return currentState;
}

bool StateMachine::isTransitionAllowed(STATE from, STATE to) noexcept {
    switch (from) {
        case STATE::INITIALIZING:
            return to == STATE::DISARMED || to == STATE::OBSERVING || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::USER_ADJUSTING_BEAMS:
            return to == STATE::DISARMED || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::CALIBRATION_LDR_THRESH:
            return to == STATE::DISARMED || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::CALIBRATION_MODULATION_FREQUENCY:
            return to == STATE::DISARMED || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::DISARMED:
            return to == STATE::USER_ADJUSTING_BEAMS || to == STATE::CALIBRATION_LDR_THRESH || to == STATE::CALIBRATION_MODULATION_FREQUENCY || to == STATE::DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST || to == STATE::DIAGNOSTIC_GPIO_DISCOVERY || to == STATE::FAULT || to == STATE::OBSERVING || to == STATE::SHUTTING_DOWN;

        case STATE::DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST:
            return to == STATE::DISARMED || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::DIAGNOSTIC_GPIO_DISCOVERY:
            return to == STATE::DISARMED || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::OBSERVING:
            return to == STATE::DISARMED || to == STATE::ALARM || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::ALARM:
            return to == STATE::DISARMED || to == STATE::OBSERVING || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::FAULT:
            return to == STATE::SHUTTING_DOWN;

        case STATE::SHUTTING_DOWN:
            return to == STATE::FAULT;

        case STATE::NONE:
            return false;
    }

    return false;
}

void StateMachine::setState(STATE state, std::string reason) noexcept {
    if (isTransitionAllowed(currentState, state)) {
        if (state == STATE::FAULT) {
            setLastFaultReason(std::move(reason));
        }
        applyState(state);
        return;
    }

    if (isTransitionAllowed(currentState, STATE::FAULT)) {
        setLastFaultReason(
            std::string("invalid transition requested: ") + StateMachine::toString(currentState) + " -> " + StateMachine::toString(state)
        );
        applyState(STATE::FAULT);
    }
}

void StateMachine::applyState(STATE state) noexcept {
    ESP_LOGI(LOG_TAG, "state change: %s -> %s", StateMachine::toString(currentState), StateMachine::toString(state));

    currentState = state;

    if (onStateChange) {
        onStateChange();
    }
}

void StateMachine::setOnStateChange(std::function<void()> callback) noexcept {
    onStateChange = std::move(callback);
}

const std::string& StateMachine::getLastFaultReason() const noexcept {
    return lastFaultReason;
}

void StateMachine::setLastFaultReason(std::string reason) noexcept {
    ESP_LOGE(LOG_TAG, "fault reason: %s", reason.c_str());
    lastFaultReason = std::move(reason);
}

const char* StateMachine::toString(STATE state) noexcept {
    switch (state) {
        case STATE::INITIALIZING: return "INITIALIZING";
        case STATE::USER_ADJUSTING_BEAMS: return "USER_ADJUSTING_BEAMS";
        case STATE::CALIBRATION_LDR_THRESH: return "CALIBRATION_LDR_THRESH";
        case STATE::CALIBRATION_MODULATION_FREQUENCY: return "CALIBRATION_MODULATION_FREQUENCY";
        case STATE::OBSERVING: return "OBSERVING";
        case STATE::DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST: return "DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST";
        case STATE::DIAGNOSTIC_GPIO_DISCOVERY: return "DIAGNOSTIC_GPIO_DISCOVERY";
        case STATE::DISARMED: return "DISARMED";
        case STATE::ALARM: return "ALARM";
        case STATE::FAULT: return "FAULT";
        case STATE::SHUTTING_DOWN: return "SHUTTING_DOWN";
        case STATE::NONE: return "NONE";
    }
    return "UNKNOWN";
}

std::optional<STATE> StateMachine::fromString(std::string_view name) noexcept {
    static constexpr std::array<STATE, 12> allStates {
        STATE::NONE, STATE::INITIALIZING, STATE::USER_ADJUSTING_BEAMS, STATE::CALIBRATION_LDR_THRESH,
        STATE::CALIBRATION_MODULATION_FREQUENCY, STATE::OBSERVING, STATE::DIAGNOSTIC_SIGNAL_NOISE_SELF_TEST,
        STATE::DIAGNOSTIC_GPIO_DISCOVERY, STATE::DISARMED, STATE::ALARM, STATE::FAULT, STATE::SHUTTING_DOWN
    };

    for (const STATE state : allStates) {
        if (name == toString(state)) {
            return state;
        }
    }
    return std::nullopt;
}
