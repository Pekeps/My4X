// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
#include "game/FactionRegistry.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(FactionRegistryTest, AddFactionReturnsIncrementingIds) {
    game::FactionRegistry registry;
    game::FactionId id1 = registry.addFaction("Rome", game::FactionType::Player, 0);
    game::FactionId id2 = registry.addFaction("Greece", game::FactionType::Player, 1);
    EXPECT_EQ(id1, 1);
    EXPECT_EQ(id2, 2);
}

TEST(FactionRegistryTest, GetFactionByIdReturnsCorrectFaction) {
    game::FactionRegistry registry;
    game::FactionId id = registry.addFaction("Rome", game::FactionType::Player, 0);
    const auto &faction = registry.getFaction(id);
    EXPECT_EQ(faction.id(), id);
    EXPECT_EQ(faction.name(), "Rome");
    EXPECT_EQ(faction.type(), game::FactionType::Player);
    EXPECT_EQ(faction.colorIndex(), 0);
}

TEST(FactionRegistryTest, GetFactionThrowsForInvalidId) {
    game::FactionRegistry registry;
    EXPECT_THROW(registry.getFaction(999), std::out_of_range);
}

TEST(FactionRegistryTest, GetMutableFaction) {
    game::FactionRegistry registry;
    game::FactionId id = registry.addFaction("Rome", game::FactionType::Player, 0);
    auto &faction = registry.getMutableFaction(id);
    faction.setName("Roman Empire");
    EXPECT_EQ(registry.getFaction(id).name(), "Roman Empire");
}

TEST(FactionRegistryTest, GetMutableFactionThrowsForInvalidId) {
    game::FactionRegistry registry;
    EXPECT_THROW(registry.getMutableFaction(999), std::out_of_range);
}

TEST(FactionRegistryTest, AllFactionsReturnsAllRegistered) {
    game::FactionRegistry registry;
    registry.addFaction("Rome", game::FactionType::Player, 0);
    registry.addFaction("Barbarians", game::FactionType::NeutralHostile, 1);
    registry.addFaction("City-State", game::FactionType::NeutralPassive, 2);
    EXPECT_EQ(registry.allFactions().size(), 3);
}

TEST(FactionRegistryTest, AllFactionsEmptyByDefault) {
    game::FactionRegistry registry;
    EXPECT_TRUE(registry.allFactions().empty());
}

TEST(FactionRegistryTest, PlayerFactionsFiltersCorrectly) {
    game::FactionRegistry registry;
    registry.addFaction("Rome", game::FactionType::Player, 0);
    registry.addFaction("Barbarians", game::FactionType::NeutralHostile, 1);
    registry.addFaction("Greece", game::FactionType::Player, 2);
    registry.addFaction("City-State", game::FactionType::NeutralPassive, 3);

    auto players = registry.playerFactions();
    EXPECT_EQ(players.size(), 2);
    EXPECT_EQ(players[0]->name(), "Rome");
    EXPECT_EQ(players[1]->name(), "Greece");
}

TEST(FactionRegistryTest, NeutralFactionsFiltersCorrectly) {
    game::FactionRegistry registry;
    registry.addFaction("Rome", game::FactionType::Player, 0);
    registry.addFaction("Barbarians", game::FactionType::NeutralHostile, 1);
    registry.addFaction("City-State", game::FactionType::NeutralPassive, 2);
    registry.addFaction("Greece", game::FactionType::Player, 3);

    auto neutrals = registry.neutralFactions();
    EXPECT_EQ(neutrals.size(), 2);
    EXPECT_EQ(neutrals[0]->name(), "Barbarians");
    EXPECT_EQ(neutrals[1]->name(), "City-State");
}

TEST(FactionRegistryTest, PlayerFactionsEmptyWhenNoPlayers) {
    game::FactionRegistry registry;
    registry.addFaction("Barbarians", game::FactionType::NeutralHostile, 0);
    EXPECT_TRUE(registry.playerFactions().empty());
}

TEST(FactionRegistryTest, NeutralFactionsEmptyWhenNoNeutrals) {
    game::FactionRegistry registry;
    registry.addFaction("Rome", game::FactionType::Player, 0);
    EXPECT_TRUE(registry.neutralFactions().empty());
}

TEST(FactionRegistryTest, NextFactionIdTracking) {
    game::FactionRegistry registry;
    EXPECT_EQ(registry.nextFactionId(), 1);
    registry.addFaction("Rome", game::FactionType::Player, 0);
    EXPECT_EQ(registry.nextFactionId(), 2);
    registry.addFaction("Greece", game::FactionType::Player, 1);
    EXPECT_EQ(registry.nextFactionId(), 3);
}

TEST(FactionRegistryTest, SetNextFactionId) {
    game::FactionRegistry registry;
    registry.setNextFactionId(10);
    game::FactionId id = registry.addFaction("Rome", game::FactionType::Player, 0);
    EXPECT_EQ(id, 10);
    EXPECT_EQ(registry.nextFactionId(), 11);
}

TEST(FactionRegistryTest, RestoreFactionPreservesId) {
    game::FactionRegistry registry;
    game::Faction faction(42, "Restored", game::FactionType::Player, 5);
    registry.restoreFaction(std::move(faction));

    const auto &restored = registry.getFaction(42);
    EXPECT_EQ(restored.id(), 42);
    EXPECT_EQ(restored.name(), "Restored");
    EXPECT_EQ(restored.type(), game::FactionType::Player);
    EXPECT_EQ(restored.colorIndex(), 5);
}

TEST(FactionRegistryTest, FactionStockpileAccessibleViaRegistry) {
    game::FactionRegistry registry;
    game::FactionId id = registry.addFaction("Rome", game::FactionType::Player, 0);
    auto &faction = registry.getMutableFaction(id);
    faction.addResources({100, 200, 300});
    EXPECT_EQ(registry.getFaction(id).stockpile(), (game::Resource{100, 200, 300}));
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
