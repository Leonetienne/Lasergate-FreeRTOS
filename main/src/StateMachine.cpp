#include "StateMachine.h"
#include "compat/esp_log_macros.h"

namespace {
    const char* stateToString(STATE state) noexcept {
        switch (state) {
            case STATE::INITIALIZING: return "INITIALIZING";
            case STATE::USER_ADJUSTING_BEAMS: return "USER_ADJUSTING_BEAMS";
            case STATE::CALIBRATION_LDR_THRESH: return "CALIBRATION_LDR_THRESH";
            case STATE::CALIBRATION_MODULATION_FREQUENCY: return "CALIBRATION_MODULATION_FREQUENCY";
            case STATE::OBSERVING: return "OBSERVING";
            case STATE::DISARMED: return "DISARMED";
            case STATE::ALARM: return "ALARM";
            case STATE::FAULT: return "FAULT";
            case STATE::SHUTTING_DOWN: return "SHUTTING_DOWN";
        }
        return "UNKNOWN";
    }
}

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
            return to == STATE::DISARMED || to == STATE::CALIBRATION_MODULATION_FREQUENCY || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::CALIBRATION_MODULATION_FREQUENCY:
            return to == STATE::DISARMED || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::DISARMED:
            return to == STATE::USER_ADJUSTING_BEAMS || to == STATE::CALIBRATION_LDR_THRESH || to == STATE::CALIBRATION_MODULATION_FREQUENCY || to == STATE::FAULT || to == STATE::OBSERVING || to == STATE::SHUTTING_DOWN;

        case STATE::OBSERVING:
            return to == STATE::DISARMED || to == STATE::ALARM || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::ALARM:
            return to == STATE::DISARMED || to == STATE::ALARM || to == STATE::FAULT || to == STATE::SHUTTING_DOWN;

        case STATE::FAULT:
            return to == STATE::SHUTTING_DOWN;

        case STATE::SHUTTING_DOWN:
            return to == STATE::FAULT;
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
            std::string("invalid transition requested: ") + stateToString(currentState) + " -> " + stateToString(state)
        );
        applyState(STATE::FAULT);
    }
}

void StateMachine::applyState(STATE state) noexcept {
    ESP_LOGI(LOG_TAG, "state change: %s -> %s", stateToString(currentState), stateToString(state));

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
