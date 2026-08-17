//
// Created by Leon Etienne on 18.03.26.
//

#ifndef LASERGATE_V2_STATEMACHINE_H
#define LASERGATE_V2_STATEMACHINE_H

#include "enum/States.h"

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
     * Sets the current state
     */
    void setState(STATE state) noexcept;

private:
    STATE currentState = STATE::INITIALIZATION;
};

#endif //LASERGATE_V2_STATEMACHINE_H
