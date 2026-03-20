// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
#include "game/City.h"
#include "game/DiplomacyManager.h"
#include "game/Faction.h"
#include "game/FactionRegistry.h"
#include "game/GameState.h"
#include "game/NeutralAI.h"
#include "game/Resource.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

// ── findMutableFaction ───────────────────────────────────────────────────────

TEST(FactionRegistryMutableFind, FindMutableFactionReturnsPointer) {
    game::FactionRegistry registry;
    game::FactionId id = registry.addFaction("Test", game::FactionType::Player, 0);
    game::Faction *faction = registry.findMutableFaction(id);
    ASSERT_NE(faction, nullptr);
    EXPECT_EQ(faction->name(), "Test");
}

TEST(FactionRegistryMutableFind, FindMutableFactionReturnsNullForUnknown) {
    game::FactionRegistry registry;
    registry.addFaction("Test", game::FactionType::Player, 0);
    game::Faction *faction = registry.findMutableFaction(999);
    EXPECT_EQ(faction, nullptr);
}

TEST(FactionRegistryMutableFind, FindMutableFactionAllowsMutation) {
    game::FactionRegistry registry;
    game::FactionId id = registry.addFaction("Test", game::FactionType::Player, 0);
    game::Faction *faction = registry.findMutableFaction(id);
    ASSERT_NE(faction, nullptr);
    faction->addResources(game::Resource{.gold = 100});
    EXPECT_EQ(faction->stockpile().gold, 100);
}

// ── Faction-aware UI data queries ────────────────────────────────────────────

class FactionUIDataTest : public ::testing::Test {
  protected:
    void SetUp() override {
        state = std::make_unique<game::GameState>(10, 10);

        // Register player faction.
        playerId = state->mutableFactionRegistry().addFaction("Romans", game::FactionType::Player, 0);

        // Register an enemy faction.
        enemyId = state->mutableFactionRegistry().addFaction("Barbarians", game::FactionType::NeutralHostile, 6);

        // Set up diplomacy.
        state->mutableDiplomacy().initializeDefaults(state->factionRegistry());
    }

    std::unique_ptr<game::GameState> state;
    game::FactionId playerId{};
    game::FactionId enemyId{};
};

TEST_F(FactionUIDataTest, PlayerFactionLookupByIdWorks) {
    const auto *faction = state->factionRegistry().findFaction(playerId);
    ASSERT_NE(faction, nullptr);
    EXPECT_EQ(faction->name(), "Romans");
    EXPECT_EQ(faction->type(), game::FactionType::Player);
}

TEST_F(FactionUIDataTest, EnemyFactionLookupByIdWorks) {
    const auto *faction = state->factionRegistry().findFaction(enemyId);
    ASSERT_NE(faction, nullptr);
    EXPECT_EQ(faction->name(), "Barbarians");
    EXPECT_EQ(faction->type(), game::FactionType::NeutralHostile);
}

TEST_F(FactionUIDataTest, DiplomacyBetweenPlayerAndHostileIsWar) {
    EXPECT_TRUE(state->diplomacy().areAtWar(playerId, enemyId));
    EXPECT_EQ(state->diplomacy().getRelation(playerId, enemyId), game::DiplomacyState::War);
}

TEST_F(FactionUIDataTest, PlayerFactionStockpileIsAccessible) {
    auto &faction = state->mutableFactionRegistry().getMutableFaction(playerId);
    faction.addResources(game::Resource{.gold = 50, .production = 20, .food = 30});
    EXPECT_EQ(faction.stockpile().gold, 50);
    EXPECT_EQ(faction.stockpile().production, 20);
    EXPECT_EQ(faction.stockpile().food, 30);
}

TEST_F(FactionUIDataTest, AllFactionsListContainsBothFactions) {
    const auto &allFactions = state->factionRegistry().allFactions();
    EXPECT_EQ(allFactions.size(), 2U);
}

TEST_F(FactionUIDataTest, UnitFactionLookupWorks) {
    game::UnitTypeRegistry unitReg;
    unitReg.registerDefaults();
    state->addUnit(std::make_unique<game::Warrior>(3, 3, unitReg, playerId));
    state->addUnit(std::make_unique<game::Warrior>(5, 5, unitReg, enemyId));

    EXPECT_EQ(state->units()[0]->factionId(), playerId);
    EXPECT_EQ(state->units()[1]->factionId(), enemyId);

    // Verify faction lookup from unit gives correct names.
    const auto *faction0 = state->factionRegistry().findFaction(state->units()[0]->factionId());
    ASSERT_NE(faction0, nullptr);
    EXPECT_EQ(faction0->name(), "Romans");

    const auto *faction1 = state->factionRegistry().findFaction(state->units()[1]->factionId());
    ASSERT_NE(faction1, nullptr);
    EXPECT_EQ(faction1->name(), "Barbarians");
}

TEST_F(FactionUIDataTest, CityFactionLookupWorks) {
    game::City city("TestCity", 3, 3, static_cast<int>(playerId));
    state->addCity(std::move(city));

    const auto &cities = state->cities();
    ASSERT_EQ(cities.size(), 1U);

    auto cityFactionId = static_cast<game::FactionId>(cities[0].factionId());
    const auto *faction = state->factionRegistry().findFaction(cityFactionId);
    ASSERT_NE(faction, nullptr);
    EXPECT_EQ(faction->name(), "Romans");
}

TEST_F(FactionUIDataTest, GarrisonCountForCityTerritory) {
    game::City city("TestCity", 3, 3, static_cast<int>(playerId));
    city.addTile(3, 4);
    state->addCity(std::move(city));

    game::UnitTypeRegistry unitReg;
    unitReg.registerDefaults();
    // Place two units inside city territory.
    state->addUnit(std::make_unique<game::Warrior>(3, 3, unitReg, playerId));
    state->addUnit(std::make_unique<game::Warrior>(3, 4, unitReg, playerId));
    // Place one unit outside.
    state->addUnit(std::make_unique<game::Warrior>(7, 7, unitReg, playerId));

    const auto &storedCity = state->cities()[0];
    int garrisonCount = 0;
    for (const auto &unit : state->units()) {
        if (unit->isAlive() && storedCity.containsTile(unit->row(), unit->col())) {
            ++garrisonCount;
        }
    }
    EXPECT_EQ(garrisonCount, 2);
}

// ── Diplomacy state labels correspond to correct relations ──────────────────

TEST_F(FactionUIDataTest, DiplomacyAllianceCanBeSet) {
    // Add a third faction and make it allied.
    game::FactionId allyId = state->mutableFactionRegistry().addFaction("Allies", game::FactionType::Player, 2);
    state->mutableDiplomacy().setRelation(playerId, allyId, game::DiplomacyState::Alliance);
    EXPECT_TRUE(state->diplomacy().areAllied(playerId, allyId));
    EXPECT_EQ(state->diplomacy().getRelation(playerId, allyId), game::DiplomacyState::Alliance);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
