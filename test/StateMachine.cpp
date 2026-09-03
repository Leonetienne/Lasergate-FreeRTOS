#include <catch2/catch_test_macros.hpp>
#include "../main/include/StateMachine.h"
#include <array>
#include <map>
#include <set>

TEST_CASE("StateMachine", "[StateMachine]") {
    StateMachine stateMachine;

    SECTION("default state is INITIALIZING") {
        REQUIRE(stateMachine.getState() == STATE::INITIALIZING);
    }

    SECTION("default lastFaultReason is empty") {
        REQUIRE(stateMachine.getLastFaultReason().empty());
    }

    SECTION("a valid transition applies the new state and fires the callback") {
        int callCount = 0;
        stateMachine.setOnStateChange([&callCount]() { ++callCount; });

        stateMachine.setState(STATE::SHUTTING_DOWN);

        REQUIRE(callCount == 1);
        REQUIRE(stateMachine.getState() == STATE::SHUTTING_DOWN);
    }

    SECTION("onStateChange is not fired before any state change") {
        int callCount = 0;
        stateMachine.setOnStateChange([&callCount]() { ++callCount; });

        REQUIRE(callCount == 0);
    }

    SECTION("the callback takes no parameters and interrogates the state machine it was registered on") {
        int callCount = 0;
        STATE observedState = STATE::INITIALIZING;

        stateMachine.setOnStateChange([&callCount, &observedState, &stateMachine]() {
            ++callCount;
            observedState = stateMachine.getState();
        });

        stateMachine.setState(STATE::OBSERVING);

        REQUIRE(callCount == 1);
        REQUIRE(observedState == STATE::OBSERVING);
    }

    SECTION("replacing the callback with an empty std::function stops it from firing") {
        int callCount = 0;
        stateMachine.setOnStateChange([&callCount]() { ++callCount; });
        stateMachine.setOnStateChange(nullptr);

        stateMachine.setState(STATE::OBSERVING);

        REQUIRE(callCount == 0);
    }

    SECTION("an invalid transition escalates to FAULT, fires the callback once and records a reason") {
        int callCount = 0;
        stateMachine.setOnStateChange([&callCount]() { ++callCount; });

        // INITIALIZING -> CALIBRATION_LDR_THRESH is not in the transition table.
        stateMachine.setState(STATE::CALIBRATION_LDR_THRESH);

        REQUIRE(callCount == 1);
        REQUIRE(stateMachine.getState() == STATE::FAULT);
        REQUIRE_FALSE(stateMachine.getLastFaultReason().empty());
    }

    SECTION("an invalid transition while already in FAULT is silently ignored") {
        stateMachine.setState(STATE::FAULT, "initial fault");

        int callCount = 0;
        stateMachine.setOnStateChange([&callCount]() { ++callCount; });

        stateMachine.setState(STATE::OBSERVING); // not allowed from FAULT

        REQUIRE(callCount == 0);
        REQUIRE(stateMachine.getState() == STATE::FAULT);
        REQUIRE(stateMachine.getLastFaultReason() == "initial fault");
    }

    SECTION("SHUTTING_DOWN -> FAULT is a valid explicit transition (e.g. free() failing during shutdown)") {
        stateMachine.setState(STATE::SHUTTING_DOWN);

        stateMachine.setState(STATE::FAULT, "free() failed during shutdown");

        REQUIRE(stateMachine.getState() == STATE::FAULT);
        REQUIRE(stateMachine.getLastFaultReason() == "free() failed during shutdown");
    }

    SECTION("an invalid transition while in SHUTTING_DOWN escalates to FAULT, since FAULT is reachable from there") {
        stateMachine.setState(STATE::SHUTTING_DOWN);

        int callCount = 0;
        stateMachine.setOnStateChange([&callCount]() { ++callCount; });

        stateMachine.setState(STATE::DISARMED); // not allowed from SHUTTING_DOWN

        REQUIRE(callCount == 1);
        REQUIRE(stateMachine.getState() == STATE::FAULT);
        REQUIRE_FALSE(stateMachine.getLastFaultReason().empty());
    }

    SECTION("ALARM -> OBSERVING is a valid transition and fires the callback") {
        stateMachine.setState(STATE::OBSERVING);
        stateMachine.setState(STATE::ALARM);

        int callCount = 0;
        stateMachine.setOnStateChange([&callCount]() { ++callCount; });

        stateMachine.setState(STATE::OBSERVING);

        REQUIRE(callCount == 1);
        REQUIRE(stateMachine.getState() == STATE::OBSERVING);
    }

    SECTION("any other same-state request (no self-loop in the table) is treated as invalid and escalates to FAULT") {
        stateMachine.setState(STATE::DISARMED);
        REQUIRE(stateMachine.getState() == STATE::DISARMED);

        stateMachine.setState(STATE::DISARMED);

        REQUIRE(stateMachine.getState() == STATE::FAULT);
    }

    SECTION("an explicit transition to FAULT records the supplied reason") {
        stateMachine.setState(STATE::FAULT, "sensor read failure");

        REQUIRE(stateMachine.getState() == STATE::FAULT);
        REQUIRE(stateMachine.getLastFaultReason() == "sensor read failure");
    }

    SECTION("setLastFaultReason/getLastFaultReason work independently of setState") {
        stateMachine.setLastFaultReason("manually recorded reason");

        REQUIRE(stateMachine.getLastFaultReason() == "manually recorded reason");
    }

    SECTION("isTransitionAllowed matches the specified transition table") {
        using S = STATE;

        const std::map<S, std::set<S>> allowed {
            {S::INITIALIZING, {S::DISARMED, S::OBSERVING, S::FAULT, S::SHUTTING_DOWN}},
            {S::USER_ADJUSTING_BEAMS, {S::DISARMED, S::FAULT, S::SHUTTING_DOWN}},
            {S::CALIBRATION_LDR_THRESH, {S::DISARMED, S::FAULT, S::SHUTTING_DOWN}},
            {S::CALIBRATION_MODULATION_FREQUENCY, {S::DISARMED, S::FAULT, S::SHUTTING_DOWN}},
            {S::DISARMED, {S::USER_ADJUSTING_BEAMS, S::CALIBRATION_LDR_THRESH, S::CALIBRATION_MODULATION_FREQUENCY, S::DIAGNOSTIC_SIGNAL_TEST_RUN, S::FAULT, S::OBSERVING, S::SHUTTING_DOWN}},
            {S::DIAGNOSTIC_SIGNAL_TEST_RUN, {S::DISARMED, S::FAULT, S::SHUTTING_DOWN}},
            {S::OBSERVING, {S::DISARMED, S::ALARM, S::FAULT, S::SHUTTING_DOWN}},
            {S::ALARM, {S::DISARMED, S::OBSERVING, S::FAULT, S::SHUTTING_DOWN}},
            {S::FAULT, {S::SHUTTING_DOWN}},
            {S::SHUTTING_DOWN, {S::FAULT}},
        };

        constexpr std::array<S, 10> allStates {
            S::INITIALIZING, S::USER_ADJUSTING_BEAMS, S::CALIBRATION_LDR_THRESH,
            S::CALIBRATION_MODULATION_FREQUENCY, S::DISARMED, S::DIAGNOSTIC_SIGNAL_TEST_RUN,
            S::OBSERVING, S::ALARM, S::FAULT, S::SHUTTING_DOWN
        };

        for (const auto from : allStates) {
            for (const auto to : allStates) {
                const bool expected = allowed.at(from).contains(to);
                INFO("from=" << static_cast<int>(from) << " to=" << static_cast<int>(to));
                REQUIRE(StateMachine::isTransitionAllowed(from, to) == expected);
            }
        }
    }
}
