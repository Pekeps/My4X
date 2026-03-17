#include "game/GameState.h"

#include <gtest/gtest.h>

TEST(GameStateTest, InitialTurnIsOne) {
    game::GameState state;
    EXPECT_EQ(state.getTurn(), 1);
}

TEST(GameStateTest, NextTurnIncrements) {
    game::GameState state;
    state.nextTurn();
    EXPECT_EQ(state.getTurn(), 2);
}
