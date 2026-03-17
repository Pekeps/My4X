#include "game/Unit.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

TEST(WarriorTest, DefaultStats) {
    game::Warrior warrior(3, 5);
    EXPECT_EQ(warrior.health(), 100);
    EXPECT_EQ(warrior.maxHealth(), 100);
    EXPECT_EQ(warrior.movement(), 2);
    EXPECT_EQ(warrior.attackPower(), 15);
    EXPECT_EQ(warrior.name(), "Warrior");
}

TEST(WarriorTest, Position) {
    game::Warrior warrior(3, 5);
    EXPECT_EQ(warrior.row(), 3);
    EXPECT_EQ(warrior.col(), 5);
}

TEST(UnitTest, MoveTo) {
    game::Warrior unit(0, 0);
    unit.moveTo(1, 2);
    EXPECT_EQ(unit.row(), 1);
    EXPECT_EQ(unit.col(), 2);
    EXPECT_EQ(unit.movementRemaining(), 1);
}

TEST(UnitTest, MovementDepletesAfterMaxMoves) {
    game::Warrior unit(0, 0);
    unit.moveTo(1, 0);
    unit.moveTo(2, 0);
    EXPECT_EQ(unit.movementRemaining(), 0);
}

TEST(UnitTest, ResetMovement) {
    game::Warrior unit(0, 0);
    unit.moveTo(1, 0);
    EXPECT_EQ(unit.movementRemaining(), 1);
    unit.resetMovement();
    EXPECT_EQ(unit.movementRemaining(), 2);
}

TEST(UnitTest, TakeDamage) {
    game::Warrior unit(0, 0);
    unit.takeDamage(30);
    EXPECT_EQ(unit.health(), 70);
    EXPECT_TRUE(unit.isAlive());
}

TEST(UnitTest, TakeFatalDamage) {
    game::Warrior unit(0, 0);
    unit.takeDamage(150);
    EXPECT_EQ(unit.health(), 0);
    EXPECT_FALSE(unit.isAlive());
}

TEST(UnitTest, TakeExactLethalDamage) {
    game::Warrior unit(0, 0);
    unit.takeDamage(100);
    EXPECT_EQ(unit.health(), 0);
    EXPECT_FALSE(unit.isAlive());
}

TEST(UnitTest, HealthNeverGoesNegative) {
    game::Warrior unit(0, 0);
    unit.takeDamage(999);
    EXPECT_EQ(unit.health(), 0);
}
