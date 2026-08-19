//
// Created by Leon Etienne on 18.03.26.
//

#ifndef LASERGATE_V2_STATEMACHINE_H
#define LASERGATE_V2_STATEMACHINE_H

#include "enum/States.h"
#include <functional>
#include <string>

class StateMachine {
public:
    StateMachine() noexcept;
    StateMachine(const StateMachine&) = delete;
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;

    /**
     * @return The current state
     */
    [[nodiscard]] STATE getState() const noexcept;

    /**
     * Sets the current state, guarded by the state transition table (see isTransitionAllowed).
     * If the transition is not allowed from the current state, the machine escalates to FAULT
     * instead (unless FAULT itself isn't reachable from the current state, e.g. while already in
     * FAULT, in which case the request is silently ignored).
     * Fires the onStateChange callback whenever a state actually gets applied.
     * @param reason Recorded via setLastFaultReason() whenever this call results in state FAULT
     */
    void setState(STATE state, std::string reason = "") noexcept;

    /**
     * @return Whether transitioning from `from` to `to` is permitted by the state transition table
     */
    [[nodiscard]] static bool isTransitionAllowed(STATE from, STATE to) noexcept;

    /**
     * Callback setter. Fired whenever setState() applies a new state.
     */
    void setOnStateChange(std::function<void()> callback) noexcept;

    /**
     * @return The reason recorded for the most recent transition into FAULT
     */
    [[nodiscard]] const std::string& getLastFaultReason() const noexcept;

    /**
     * Sets the reason recorded for the most recent transition into FAULT
     */
    void setLastFaultReason(std::string reason) noexcept;

private:
    void applyState(STATE state) noexcept;

    STATE currentState = STATE::INITIALIZING;
    std::function<void()> onStateChange;
    std::string lastFaultReason;
};

#endif //LASERGATE_V2_STATEMACHINE_H
