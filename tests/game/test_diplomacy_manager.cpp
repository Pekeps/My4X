// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"

#include <gtest/gtest.h>

// -- Basic state tracking ------------------------------------------------

TEST(DiplomacyManagerTest, DefaultRelationIsPeace) {
    game::DiplomacyManager mgr;
    EXPECT_EQ(mgr.getRelation(1, 2), game::DiplomacyState::Peace);
}

TEST(DiplomacyManagerTest, SelfRelationIsPeace) {
    game::DiplomacyManager mgr;
    EXPECT_EQ(mgr.getRelation(1, 1), game::DiplomacyState::Peace);
}

TEST(DiplomacyManagerTest, SetRelationStoresState) {
    game::DiplomacyManager mgr;
    mgr.setRelation(1, 2, game::DiplomacyState::War);
    EXPECT_EQ(mgr.getRelation(1, 2), game::DiplomacyState::War);
}

TEST(DiplomacyManagerTest, RelationIsSymmetric) {
    game::DiplomacyManager mgr;
    mgr.setRelation(3, 1, game::DiplomacyState::Alliance);
    EXPECT_EQ(mgr.getRelation(1, 3), game::DiplomacyState::Alliance);
    EXPECT_EQ(mgr.getRelation(3, 1), game::DiplomacyState::Alliance);
}

TEST(DiplomacyManagerTest, SetRelationOnSelfIsNoop) {
    game::DiplomacyManager mgr;
    mgr.setRelation(5, 5, game::DiplomacyState::War);
    EXPECT_EQ(mgr.getRelation(5, 5), game::DiplomacyState::Peace);
    EXPECT_EQ(mgr.size(), 0);
}

// -- State transitions ---------------------------------------------------

TEST(DiplomacyManagerTest, TransitionWarToPeace) {
    game::DiplomacyManager mgr;
    mgr.setRelation(1, 2, game::DiplomacyState::War);
    EXPECT_TRUE(mgr.areAtWar(1, 2));

    mgr.setRelation(1, 2, game::DiplomacyState::Peace);
    EXPECT_FALSE(mgr.areAtWar(1, 2));
    EXPECT_EQ(mgr.getRelation(1, 2), game::DiplomacyState::Peace);
}

TEST(DiplomacyManagerTest, TransitionPeaceToAlliance) {
    game::DiplomacyManager mgr;
    mgr.setRelation(1, 2, game::DiplomacyState::Peace);
    mgr.setRelation(1, 2, game::DiplomacyState::Alliance);
    EXPECT_TRUE(mgr.areAllied(1, 2));
}

TEST(DiplomacyManagerTest, TransitionAllianceToWar) {
    game::DiplomacyManager mgr;
    mgr.setRelation(1, 2, game::DiplomacyState::Alliance);
    EXPECT_TRUE(mgr.areAllied(1, 2));

    mgr.setRelation(1, 2, game::DiplomacyState::War);
    EXPECT_FALSE(mgr.areAllied(1, 2));
    EXPECT_TRUE(mgr.areAtWar(1, 2));
}

// -- Convenience methods -------------------------------------------------

TEST(DiplomacyManagerTest, AreAtWarReturnsTrueOnlyForWar) {
    game::DiplomacyManager mgr;
    mgr.setRelation(1, 2, game::DiplomacyState::War);
    mgr.setRelation(1, 3, game::DiplomacyState::Peace);
    mgr.setRelation(1, 4, game::DiplomacyState::Alliance);

    EXPECT_TRUE(mgr.areAtWar(1, 2));
    EXPECT_FALSE(mgr.areAtWar(1, 3));
    EXPECT_FALSE(mgr.areAtWar(1, 4));
}

TEST(DiplomacyManagerTest, AreAlliedReturnsTrueOnlyForAlliance) {
    game::DiplomacyManager mgr;
    mgr.setRelation(1, 2, game::DiplomacyState::Alliance);
    mgr.setRelation(1, 3, game::DiplomacyState::Peace);
    mgr.setRelation(1, 4, game::DiplomacyState::War);

    EXPECT_TRUE(mgr.areAllied(1, 2));
    EXPECT_FALSE(mgr.areAllied(1, 3));
    EXPECT_FALSE(mgr.areAllied(1, 4));
}

// -- Default initialization from FactionRegistry -------------------------

TEST(DiplomacyManagerTest, DefaultsPlayerVsPlayerIsWar) {
    game::FactionRegistry registry;
    registry.addFaction("Rome", game::FactionType::Player, 0);
    registry.addFaction("Greece", game::FactionType::Player, 1);

    game::DiplomacyManager mgr;
    mgr.initializeDefaults(registry);

    EXPECT_TRUE(mgr.areAtWar(1, 2));
}

TEST(DiplomacyManagerTest, DefaultsPlayerVsNeutralHostileIsWar) {
    game::FactionRegistry registry;
    auto playerId = registry.addFaction("Rome", game::FactionType::Player, 0);
    auto hostileId = registry.addFaction("Barbarians", game::FactionType::NeutralHostile, 1);

    game::DiplomacyManager mgr;
    mgr.initializeDefaults(registry);

    EXPECT_TRUE(mgr.areAtWar(playerId, hostileId));
}

TEST(DiplomacyManagerTest, DefaultsPlayerVsNeutralPassiveIsPeace) {
    game::FactionRegistry registry;
    auto playerId = registry.addFaction("Rome", game::FactionType::Player, 0);
    auto passiveId = registry.addFaction("CityState", game::FactionType::NeutralPassive, 1);

    game::DiplomacyManager mgr;
    mgr.initializeDefaults(registry);

    EXPECT_FALSE(mgr.areAtWar(playerId, passiveId));
    EXPECT_EQ(mgr.getRelation(playerId, passiveId), game::DiplomacyState::Peace);
}

TEST(DiplomacyManagerTest, DefaultsNeutralHostileVsNeutralPassiveIsWar) {
    game::FactionRegistry registry;
    auto hostileId = registry.addFaction("Barbarians", game::FactionType::NeutralHostile, 0);
    auto passiveId = registry.addFaction("CityState", game::FactionType::NeutralPassive, 1);

    game::DiplomacyManager mgr;
    mgr.initializeDefaults(registry);

    EXPECT_TRUE(mgr.areAtWar(hostileId, passiveId));
}

TEST(DiplomacyManagerTest, DefaultsNeutralHostileVsNeutralHostileIsPeace) {
    game::FactionRegistry registry;
    auto h1 = registry.addFaction("Barbarians", game::FactionType::NeutralHostile, 0);
    auto h2 = registry.addFaction("Pirates", game::FactionType::NeutralHostile, 1);

    game::DiplomacyManager mgr;
    mgr.initializeDefaults(registry);

    EXPECT_EQ(mgr.getRelation(h1, h2), game::DiplomacyState::Peace);
}

TEST(DiplomacyManagerTest, DefaultsNeutralPassiveVsNeutralPassiveIsPeace) {
    game::FactionRegistry registry;
    auto p1 = registry.addFaction("CityState1", game::FactionType::NeutralPassive, 0);
    auto p2 = registry.addFaction("CityState2", game::FactionType::NeutralPassive, 1);

    game::DiplomacyManager mgr;
    mgr.initializeDefaults(registry);

    EXPECT_EQ(mgr.getRelation(p1, p2), game::DiplomacyState::Peace);
}

TEST(DiplomacyManagerTest, DefaultsAllFactionTypes) {
    game::FactionRegistry registry;
    auto player1 = registry.addFaction("Rome", game::FactionType::Player, 0);
    auto player2 = registry.addFaction("Greece", game::FactionType::Player, 1);
    auto hostile = registry.addFaction("Barbarians", game::FactionType::NeutralHostile, 2);
    auto passive = registry.addFaction("CityState", game::FactionType::NeutralPassive, 3);

    game::DiplomacyManager mgr;
    mgr.initializeDefaults(registry);

    // Player vs Player: War
    EXPECT_TRUE(mgr.areAtWar(player1, player2));

    // Player vs NeutralHostile: War
    EXPECT_TRUE(mgr.areAtWar(player1, hostile));
    EXPECT_TRUE(mgr.areAtWar(player2, hostile));

    // Player vs NeutralPassive: Peace
    EXPECT_EQ(mgr.getRelation(player1, passive), game::DiplomacyState::Peace);
    EXPECT_EQ(mgr.getRelation(player2, passive), game::DiplomacyState::Peace);

    // NeutralHostile vs NeutralPassive: War
    EXPECT_TRUE(mgr.areAtWar(hostile, passive));
}

TEST(DiplomacyManagerTest, DefaultsCanBeOverridden) {
    game::FactionRegistry registry;
    auto player1 = registry.addFaction("Rome", game::FactionType::Player, 0);
    auto player2 = registry.addFaction("Greece", game::FactionType::Player, 1);

    game::DiplomacyManager mgr;
    mgr.initializeDefaults(registry);

    EXPECT_TRUE(mgr.areAtWar(player1, player2));

    // Override to alliance
    mgr.setRelation(player1, player2, game::DiplomacyState::Alliance);
    EXPECT_TRUE(mgr.areAllied(player1, player2));
    EXPECT_FALSE(mgr.areAtWar(player1, player2));
}

// -- Size and clear ------------------------------------------------------

TEST(DiplomacyManagerTest, SizeTracksStoredRelations) {
    game::DiplomacyManager mgr;
    EXPECT_EQ(mgr.size(), 0);

    mgr.setRelation(1, 2, game::DiplomacyState::War);
    EXPECT_EQ(mgr.size(), 1);

    mgr.setRelation(1, 3, game::DiplomacyState::Alliance);
    EXPECT_EQ(mgr.size(), 2);

    // Overwriting an existing relation doesn't increase size
    mgr.setRelation(1, 2, game::DiplomacyState::Peace);
    EXPECT_EQ(mgr.size(), 2);
}

TEST(DiplomacyManagerTest, ClearRemovesAllRelations) {
    game::DiplomacyManager mgr;
    mgr.setRelation(1, 2, game::DiplomacyState::War);
    mgr.setRelation(3, 4, game::DiplomacyState::Alliance);
    EXPECT_EQ(mgr.size(), 2);

    mgr.clear();
    EXPECT_EQ(mgr.size(), 0);
    EXPECT_EQ(mgr.getRelation(1, 2), game::DiplomacyState::Peace);
}

// -- Multiple pairs don't interfere --------------------------------------

TEST(DiplomacyManagerTest, IndependentPairsDoNotInterfere) {
    game::DiplomacyManager mgr;
    mgr.setRelation(1, 2, game::DiplomacyState::War);
    mgr.setRelation(1, 3, game::DiplomacyState::Alliance);
    mgr.setRelation(2, 3, game::DiplomacyState::Peace);

    EXPECT_EQ(mgr.getRelation(1, 2), game::DiplomacyState::War);
    EXPECT_EQ(mgr.getRelation(1, 3), game::DiplomacyState::Alliance);
    EXPECT_EQ(mgr.getRelation(2, 3), game::DiplomacyState::Peace);
}

// -- GameState integration -----------------------------------------------

#include "game/GameState.h"

TEST(DiplomacyManagerTest, AccessibleFromGameState) {
    game::GameState state(4, 4);
    state.mutableDiplomacy().setRelation(1, 2, game::DiplomacyState::War);
    EXPECT_TRUE(state.diplomacy().areAtWar(1, 2));
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
