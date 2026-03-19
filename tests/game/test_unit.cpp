// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

namespace {

/// Shared registry for all unit tests.
game::UnitTypeRegistry makeTestRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

} // namespace

TEST(WarriorTest, DefaultStats) {
    auto reg = makeTestRegistry();
    game::Warrior warrior(3, 5, reg);
    EXPECT_EQ(warrior.health(), 100);
    EXPECT_EQ(warrior.maxHealth(), 100);
    EXPECT_EQ(warrior.movement(), 2);
    EXPECT_EQ(warrior.attack(), 15);
    EXPECT_EQ(warrior.defense(), 10);
    EXPECT_EQ(warrior.attackRange(), 1);
    EXPECT_EQ(warrior.name(), "Warrior");
}

TEST(WarriorTest, Position) {
    auto reg = makeTestRegistry();
    game::Warrior warrior(3, 5, reg);
    EXPECT_EQ(warrior.row(), 3);
    EXPECT_EQ(warrior.col(), 5);
}

TEST(UnitTest, MoveTo) {
    auto reg = makeTestRegistry();
    game::Warrior unit(0, 0, reg);
    unit.moveTo(1, 2);
    EXPECT_EQ(unit.row(), 1);
    EXPECT_EQ(unit.col(), 2);
    EXPECT_EQ(unit.movementRemaining(), 1);
}

TEST(UnitTest, MovementDepletesAfterMaxMoves) {
    auto reg = makeTestRegistry();
    game::Warrior unit(0, 0, reg);
    unit.moveTo(1, 0);
    unit.moveTo(2, 0);
    EXPECT_EQ(unit.movementRemaining(), 0);
}

TEST(UnitTest, ResetMovement) {
    auto reg = makeTestRegistry();
    game::Warrior unit(0, 0, reg);
    unit.moveTo(1, 0);
    EXPECT_EQ(unit.movementRemaining(), 1);
    unit.resetMovement();
    EXPECT_EQ(unit.movementRemaining(), 2);
}

TEST(UnitTest, TakeDamage) {
    auto reg = makeTestRegistry();
    game::Warrior unit(0, 0, reg);
    unit.takeDamage(30);
    EXPECT_EQ(unit.health(), 70);
    EXPECT_TRUE(unit.isAlive());
}

TEST(UnitTest, TakeFatalDamage) {
    auto reg = makeTestRegistry();
    game::Warrior unit(0, 0, reg);
    unit.takeDamage(150);
    EXPECT_EQ(unit.health(), 0);
    EXPECT_FALSE(unit.isAlive());
}

TEST(UnitTest, TakeExactLethalDamage) {
    auto reg = makeTestRegistry();
    game::Warrior unit(0, 0, reg);
    unit.takeDamage(100);
    EXPECT_EQ(unit.health(), 0);
    EXPECT_FALSE(unit.isAlive());
}

TEST(UnitTest, HealthNeverGoesNegative) {
    auto reg = makeTestRegistry();
    game::Warrior unit(0, 0, reg);
    unit.takeDamage(999);
    EXPECT_EQ(unit.health(), 0);
}

TEST(UnitTest, TemplateReference) {
    auto reg = makeTestRegistry();
    game::Warrior unit(0, 0, reg);
    const game::UnitTemplate &tmpl = unit.unitTemplate();
    EXPECT_EQ(tmpl.name, "Warrior");
    EXPECT_EQ(tmpl.maxHealth, 100);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
