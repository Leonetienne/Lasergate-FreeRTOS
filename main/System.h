//
// Created by Leon Etienne on 18.03.26.
//

#ifndef LASERGATE_V2_SYSTEM_H
#define LASERGATE_V2_SYSTEM_H

#include "StateMachine.h"

/**
 * System entrypoint and main runtime
 */
class System {
public:
    System() noexcept;
    System(const System&) = delete;
    System(System&&) = delete;
    System& operator=(const System&) = delete;

private:
    StateMachine stateMachine;
};


#endif //LASERGATE_V2_SYSTEM_H
