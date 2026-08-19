#ifndef LASERGATE_TESTS_GATE_H
#define LASERGATE_TESTS_GATE_H

#include "GateModule.h"
#include <array>

/**
 * A gate aggregates multiple gate modules into a single gate
 */
class Gate {
public:
    Gate(StateMachine& stateMachine, std::array<GateModule, 1> gateModules) noexcept;
    Gate(const Gate&) = delete;
    Gate(Gate&&) = delete;
    Gate& operator=(const Gate&) = delete;
    Gate& operator=(Gate&&) = delete;
    ~Gate() noexcept;

    /**
      * Will initialize the gate
      * @return Success state
      */
    bool initialize() noexcept;

    /**
     * Will free all resources acquired by this object
     * @return Success state
     */
    bool free() noexcept;

    /**
     * @return Whether this gate is ready for operation
     */
    [[nodiscard]] bool isReady() const noexcept;

    /**
     * Call every 10ms
     */
    void fixedUpdate() noexcept;

    /**
     * Gets called right after system state changes
     */
    void onStateChange() noexcept;

private:
    bool isInitialized = false;

    StateMachine& stateMachine;
    std::array<GateModule, 1>& modules;
};


#endif //LASERGATE_TESTS_GATE_H
