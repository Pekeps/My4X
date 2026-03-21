// tests/game/test_hex_direction.cpp
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/HexDirection.h"
#include <gtest/gtest.h>

TEST(HexDirectionTest, OppositeDirections) {
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::NE), game::HexDirection::SW);
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::E),  game::HexDirection::W);
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::SE), game::HexDirection::NW);
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::SW), game::HexDirection::NE);
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::W),  game::HexDirection::E);
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::NW), game::HexDirection::SE);
}

TEST(HexDirectionTest, DoubleOppositeIsIdentity) {
    for (auto dir : game::ALL_DIRECTIONS) {
        EXPECT_EQ(game::oppositeDirection(game::oppositeDirection(dir)), dir);
    }
}

TEST(HexDirectionTest, NeighborCoordEvenRow) {
    auto ne = game::neighborCoord(0, 2, game::HexDirection::NE, 10, 10);
    EXPECT_FALSE(ne.has_value()); // row -1 is out of bounds

    auto e = game::neighborCoord(0, 2, game::HexDirection::E, 10, 10);
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(e->first, 0);
    EXPECT_EQ(e->second, 3);

    auto se = game::neighborCoord(0, 2, game::HexDirection::SE, 10, 10);
    ASSERT_TRUE(se.has_value());
    EXPECT_EQ(se->first, 1);
    EXPECT_EQ(se->second, 2);
}

TEST(HexDirectionTest, NeighborCoordOddRow) {
    auto ne = game::neighborCoord(1, 2, game::HexDirection::NE, 10, 10);
    ASSERT_TRUE(ne.has_value());
    EXPECT_EQ(ne->first, 0);
    EXPECT_EQ(ne->second, 3);

    auto sw = game::neighborCoord(1, 2, game::HexDirection::SW, 10, 10);
    ASSERT_TRUE(sw.has_value());
    EXPECT_EQ(sw->first, 2);
    EXPECT_EQ(sw->second, 2);
}

TEST(HexDirectionTest, NeighborOutOfBounds) {
    auto result = game::neighborCoord(0, 0, game::HexDirection::W, 5, 5);
    EXPECT_FALSE(result.has_value());
}

TEST(HexDirectionTest, AllDirectionsHasSixEntries) {
    EXPECT_EQ(game::ALL_DIRECTIONS.size(), 6);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
