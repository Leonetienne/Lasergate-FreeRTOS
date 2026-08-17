#include <catch2/catch_test_macros.hpp>
#include "../main/include/StateMachine.h"

TEST_CASE("StateMachine", "[StateMachine]") {
    StateMachine stateMachine;

    SECTION("default state is INITIALIZATION") {
        REQUIRE(stateMachine.getState() == STATE::INITIALIZATION);
    }

    SECTION("setState changes the current state") {
        stateMachine.setState(STATE::SHUTTING_DOWN);
        REQUIRE(stateMachine.getState() == STATE::SHUTTING_DOWN);
    }
}
