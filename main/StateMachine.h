//
// Created by Leon Etienne on 18.03.26.
//

#ifndef LASERGATE_V2_STATEMACHINE_H
#define LASERGATE_V2_STATEMACHINE_H

#include "States.h"

class StateMachine {
public:
    StateMachine() noexcept;
    StateMachine(const StateMachine&) = delete;
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;

private:
    STATE currentState;
};

#endif //LASERGATE_V2_STATEMACHINE_H
