// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/PredictionManager.h"

#include "game/AttackAction.h"
#include "game/CaptureAction.h"
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
#include "game/GameSession.h"
#include "game/GameState.h"
#include "game/MoveAction.h"
#include "game/TerrainType.h"
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include <gtest/gtest.h>

#include <memory>

namespace {

/// Build a registry with the default unit types.
game::UnitTypeRegistry makeRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

/// Helper to set up a session with two player factions at war, suitable for
/// prediction manager tests.
struct PredictionSetup {
    game::GameSession session;
    game::PredictionManager prediction;
    game::FactionId factionA = 0;
    game::FactionId factionB = 0;
    game::PlayerId playerA = 1;
    game::PlayerId playerB = 2;

    PredictionSetup() : session(20, 20), prediction(session) {
        auto &state = session.mutableState();
        auto &registry = state.mutableFactionRegistry();
        factionA = registry.addFaction("FactionA", game::FactionType::Player, 0);
        factionB = registry.addFaction("FactionB", game::FactionType::Player, 1);

        // Set factions at war.
        state.mutableDiplomacy().setRelation(factionA, factionB, game::DiplomacyState::War);

        // Register players.
        session.addPlayer(playerA, factionA);
        session.addPlayer(playerB, factionB);

        // Set all tiles to Plains for deterministic tests.
        for (int r = 0; r < state.map().height(); ++r) {
            for (int c = 0; c < state.map().width(); ++c) {
                state.mutableMap().tile(r, c).setTerrainType(game::TerrainType::Plains);
            }
        }
    }
};

// ============================================================================
// Construction and initial state
// ============================================================================

TEST(PredictionManager, InitialState) {
    game::GameSession session(10, 10);
    game::PredictionManager pm(session);

    EXPECT_EQ(pm.pendingCount(), 0);
    EXPECT_EQ(pm.nextSequenceId(), 1);
    EXPECT_FALSE(pm.isPending(1));
}

// ============================================================================
// predictAction tests
// ============================================================================

TEST(PredictionManager, PredictMoveActionSucceeds) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Add a warrior at (2, 2).
    auto warrior = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // Predict a move to adjacent tile (2, 3).
    game::MoveAction move(0, 2, 3);
    auto seqId = setup.prediction.predictAction(setup.playerA, move);

    ASSERT_TRUE(seqId.has_value());
    EXPECT_EQ(seqId.value(), 1);
    EXPECT_EQ(setup.prediction.pendingCount(), 1);
    EXPECT_TRUE(setup.prediction.isPending(1));

    // Verify the move was applied optimistically.
    EXPECT_EQ(state.units()[0]->row(), 2);
    EXPECT_EQ(state.units()[0]->col(), 3);
}

TEST(PredictionManager, PredictActionSequenceIncrementsId) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Add a warrior with enough movement for multiple moves.
    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // First prediction.
    game::MoveAction move1(0, 5, 6);
    auto seq1 = setup.prediction.predictAction(setup.playerA, move1);
    ASSERT_TRUE(seq1.has_value());
    EXPECT_EQ(seq1.value(), 1);

    // Second prediction (warrior now at 5,6 with movement remaining).
    game::MoveAction move2(0, 5, 7);
    auto seq2 = setup.prediction.predictAction(setup.playerA, move2);
    ASSERT_TRUE(seq2.has_value());
    EXPECT_EQ(seq2.value(), 2);

    EXPECT_EQ(setup.prediction.pendingCount(), 2);
    EXPECT_EQ(setup.prediction.nextSequenceId(), 3);
}

TEST(PredictionManager, PredictInvalidActionReturnsNullopt) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Add a warrior with no movement remaining.
    auto warrior = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    warrior->setMovementRemaining(0);
    state.addUnit(std::move(warrior));

    game::MoveAction move(0, 2, 3);
    auto seqId = setup.prediction.predictAction(setup.playerA, move);

    EXPECT_FALSE(seqId.has_value());
    EXPECT_EQ(setup.prediction.pendingCount(), 0);
}

TEST(PredictionManager, PredictActionForInvalidPlayerReturnsNullopt) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::MoveAction move(0, 2, 3);
    // Use a non-existent player ID.
    auto seqId = setup.prediction.predictAction(99, move);

    EXPECT_FALSE(seqId.has_value());
    EXPECT_EQ(setup.prediction.pendingCount(), 0);
}

// ============================================================================
// confirmAction tests
// ============================================================================

TEST(PredictionManager, ConfirmActionRemovesPending) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::MoveAction move(0, 2, 3);
    auto seqId = setup.prediction.predictAction(setup.playerA, move);
    ASSERT_TRUE(seqId.has_value());

    EXPECT_TRUE(setup.prediction.confirmAction(seqId.value()));
    EXPECT_EQ(setup.prediction.pendingCount(), 0);
    EXPECT_FALSE(setup.prediction.isPending(seqId.value()));

    // Unit should remain at the predicted position after confirmation.
    EXPECT_EQ(state.units()[0]->row(), 2);
    EXPECT_EQ(state.units()[0]->col(), 3);
}

TEST(PredictionManager, ConfirmNonExistentActionReturnsFalse) {
    PredictionSetup setup;
    EXPECT_FALSE(setup.prediction.confirmAction(999));
}

TEST(PredictionManager, ConfirmFirstOfMultiplePredictions) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::MoveAction move1(0, 5, 6);
    auto seq1 = setup.prediction.predictAction(setup.playerA, move1);
    ASSERT_TRUE(seq1.has_value());

    game::MoveAction move2(0, 5, 7);
    auto seq2 = setup.prediction.predictAction(setup.playerA, move2);
    ASSERT_TRUE(seq2.has_value());

    // Confirm the first one.
    EXPECT_TRUE(setup.prediction.confirmAction(seq1.value()));
    EXPECT_EQ(setup.prediction.pendingCount(), 1);
    EXPECT_FALSE(setup.prediction.isPending(seq1.value()));
    EXPECT_TRUE(setup.prediction.isPending(seq2.value()));
}

// ============================================================================
// rejectAction tests
// ============================================================================

TEST(PredictionManager, RejectActionRollsBackState) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(3, 3, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // Capture original movement.
    int originalMovement = state.units()[0]->movementRemaining();

    game::MoveAction move(0, 3, 4);
    auto seqId = setup.prediction.predictAction(setup.playerA, move);
    ASSERT_TRUE(seqId.has_value());

    // Verify unit moved.
    EXPECT_EQ(state.units()[0]->col(), 4);

    // Reject the prediction.
    EXPECT_TRUE(setup.prediction.rejectAction(seqId.value()));
    EXPECT_EQ(setup.prediction.pendingCount(), 0);

    // Unit should be back at original position.
    EXPECT_EQ(state.units()[0]->row(), 3);
    EXPECT_EQ(state.units()[0]->col(), 3);
    EXPECT_EQ(state.units()[0]->movementRemaining(), originalMovement);
}

TEST(PredictionManager, RejectNonExistentActionReturnsFalse) {
    PredictionSetup setup;
    EXPECT_FALSE(setup.prediction.rejectAction(999));
}

TEST(PredictionManager, RejectFirstOfTwoPredictionsReappliesSecond) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Warrior has movement=2 on Plains (cost 1 each), so two moves per turn.
    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // Two predictions: (5,5) -> (5,6) -> (5,7)
    game::MoveAction move1(0, 5, 6);
    auto seq1 = setup.prediction.predictAction(setup.playerA, move1);
    ASSERT_TRUE(seq1.has_value());

    game::MoveAction move2(0, 5, 7);
    auto seq2 = setup.prediction.predictAction(setup.playerA, move2);
    ASSERT_TRUE(seq2.has_value());

    EXPECT_EQ(setup.prediction.pendingCount(), 2);
    EXPECT_EQ(state.units()[0]->col(), 7);

    // Reject the first prediction. This should rollback both, remove the
    // first, then re-apply the second on the original state.
    // After rolling back to (5,5): move2 targets (5,7) which is 2 distance away,
    // so re-apply fails (not adjacent). The second prediction is dropped.
    EXPECT_TRUE(setup.prediction.rejectAction(seq1.value()));

    // The unit should be back at (5,5) since re-applying move2 from (5,5) to (5,7)
    // fails (not adjacent).
    EXPECT_EQ(state.units()[0]->row(), 5);
    EXPECT_EQ(state.units()[0]->col(), 5);
    EXPECT_EQ(setup.prediction.pendingCount(), 0);
}

TEST(PredictionManager, RejectSecondOfTwoPredictionsKeepsFirst) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Warrior has movement=2 on Plains (cost 1 each), so two moves per turn.
    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // Two predictions: (5,5) -> (5,6) -> (5,7)
    game::MoveAction move1(0, 5, 6);
    auto seq1 = setup.prediction.predictAction(setup.playerA, move1);
    ASSERT_TRUE(seq1.has_value());

    game::MoveAction move2(0, 5, 7);
    auto seq2 = setup.prediction.predictAction(setup.playerA, move2);
    ASSERT_TRUE(seq2.has_value());

    EXPECT_EQ(setup.prediction.pendingCount(), 2);
    EXPECT_EQ(state.units()[0]->col(), 7);

    // Reject the second prediction. Only the second is rolled back.
    EXPECT_TRUE(setup.prediction.rejectAction(seq2.value()));

    // The first prediction should still be applied: unit at (5,6).
    EXPECT_EQ(state.units()[0]->row(), 5);
    EXPECT_EQ(state.units()[0]->col(), 6);
    EXPECT_EQ(setup.prediction.pendingCount(), 1);
    EXPECT_TRUE(setup.prediction.isPending(seq1.value()));
}

// ============================================================================
// reconcile tests
// ============================================================================

TEST(PredictionManager, ReconcileWithServerState) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(3, 3, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // Apply a prediction.
    game::MoveAction move(0, 3, 4);
    auto seqId = setup.prediction.predictAction(setup.playerA, move);
    ASSERT_TRUE(seqId.has_value());

    // Create a "server state" that has the unit at a different position.
    game::GameState serverState(20, 20);
    // Set terrain to Plains.
    for (int r = 0; r < serverState.map().height(); ++r) {
        for (int c = 0; c < serverState.map().width(); ++c) {
            serverState.mutableMap().tile(r, c).setTerrainType(game::TerrainType::Plains);
        }
    }
    auto serverWarrior = std::make_unique<game::Warrior>(3, 3, unitReg, setup.factionA);
    serverState.addUnit(std::move(serverWarrior));
    serverState.setTurn(2);

    // Reconcile. The local state should adopt the server's positions,
    // then re-apply the unconfirmed move.
    std::size_t reapplied = setup.prediction.reconcile(serverState);

    // The move from (3,3) to (3,4) should be re-applied on top of server state.
    EXPECT_EQ(reapplied, 1);
    EXPECT_EQ(state.units()[0]->col(), 4);
    EXPECT_EQ(state.getTurn(), 2);
}

TEST(PredictionManager, ReconcileNoPendingPredictions) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(3, 3, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // Create a server state with the unit at a different position.
    game::GameState serverState(20, 20);
    auto serverWarrior = std::make_unique<game::Warrior>(7, 7, unitReg, setup.factionA);
    serverState.addUnit(std::move(serverWarrior));
    serverState.setTurn(5);

    std::size_t reapplied = setup.prediction.reconcile(serverState);
    EXPECT_EQ(reapplied, 0);
    EXPECT_EQ(state.units()[0]->row(), 7);
    EXPECT_EQ(state.units()[0]->col(), 7);
    EXPECT_EQ(state.getTurn(), 5);
}

TEST(PredictionManager, ReconcileDropsInvalidPredictions) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(3, 3, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // Predict a move from (3,3) to (3,4).
    game::MoveAction move(0, 3, 4);
    auto seqId = setup.prediction.predictAction(setup.playerA, move);
    ASSERT_TRUE(seqId.has_value());

    // Create a server state with the unit at (10,10) — far from (3,4), so
    // re-applying the prediction (move to 3,4) from (10,10) will fail (not adjacent).
    game::GameState serverState(20, 20);
    for (int r = 0; r < serverState.map().height(); ++r) {
        for (int c = 0; c < serverState.map().width(); ++c) {
            serverState.mutableMap().tile(r, c).setTerrainType(game::TerrainType::Plains);
        }
    }
    auto serverWarrior = std::make_unique<game::Warrior>(10, 10, unitReg, setup.factionA);
    serverState.addUnit(std::move(serverWarrior));

    std::size_t reapplied = setup.prediction.reconcile(serverState);
    EXPECT_EQ(reapplied, 0);
    EXPECT_EQ(setup.prediction.pendingCount(), 0);

    // Unit should be at the server's position.
    EXPECT_EQ(state.units()[0]->row(), 10);
    EXPECT_EQ(state.units()[0]->col(), 10);
}

// ============================================================================
// clearPredictions tests
// ============================================================================

TEST(PredictionManager, ClearPredictions) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::MoveAction move(0, 2, 3);
    auto seqId = setup.prediction.predictAction(setup.playerA, move);
    ASSERT_TRUE(seqId.has_value());
    EXPECT_EQ(setup.prediction.pendingCount(), 1);

    setup.prediction.clearPredictions();
    EXPECT_EQ(setup.prediction.pendingCount(), 0);

    // Note: clearPredictions does NOT rollback — the unit stays where it moved.
    EXPECT_EQ(state.units()[0]->col(), 3);
}

// ============================================================================
// isPending tests
// ============================================================================

TEST(PredictionManager, IsPendingForVariousStates) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(4, 4, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    EXPECT_FALSE(setup.prediction.isPending(1));

    game::MoveAction move(0, 4, 5);
    auto seqId = setup.prediction.predictAction(setup.playerA, move);
    ASSERT_TRUE(seqId.has_value());

    EXPECT_TRUE(setup.prediction.isPending(seqId.value()));
    EXPECT_FALSE(setup.prediction.isPending(seqId.value() + 1));

    setup.prediction.confirmAction(seqId.value());
    EXPECT_FALSE(setup.prediction.isPending(seqId.value()));
}

// ============================================================================
// Attack prediction tests
// ============================================================================

TEST(PredictionManager, PredictAttackAction) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Add adjacent units from different factions.
    auto attacker = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    auto target = std::make_unique<game::Warrior>(2, 3, unitReg, setup.factionB);
    state.addUnit(std::move(attacker));
    state.addUnit(std::move(target));

    int originalAttackerHealth = state.units()[0]->health();
    int originalTargetHealth = state.units()[1]->health();

    game::AttackAction attack(0, 1);
    auto seqId = setup.prediction.predictAction(setup.playerA, attack);
    ASSERT_TRUE(seqId.has_value());
    EXPECT_EQ(setup.prediction.pendingCount(), 1);

    // After attack, at least one unit should have taken damage.
    bool someDamageDealt = (state.units()[0]->health() < originalAttackerHealth) ||
                           (state.units().size() < 2 || state.units()[1]->health() < originalTargetHealth);
    EXPECT_TRUE(someDamageDealt);
}

// ============================================================================
// Sequence ID tracking tests
// ============================================================================

TEST(PredictionManager, SequenceIdsAreMonotonicallyIncreasing) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    // Add a warrior with plenty of movement.
    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::MoveAction move1(0, 5, 6);
    auto seq1 = setup.prediction.predictAction(setup.playerA, move1);

    game::MoveAction move2(0, 5, 7);
    auto seq2 = setup.prediction.predictAction(setup.playerA, move2);

    ASSERT_TRUE(seq1.has_value());
    ASSERT_TRUE(seq2.has_value());
    EXPECT_LT(seq1.value(), seq2.value());
}

TEST(PredictionManager, SequenceCounterNotResetByConfirm) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::MoveAction move1(0, 5, 6);
    auto seq1 = setup.prediction.predictAction(setup.playerA, move1);
    ASSERT_TRUE(seq1.has_value());

    setup.prediction.confirmAction(seq1.value());

    // Next prediction should use the next sequence number, not reset.
    game::MoveAction move2(0, 5, 7);
    auto seq2 = setup.prediction.predictAction(setup.playerA, move2);
    ASSERT_TRUE(seq2.has_value());
    EXPECT_EQ(seq2.value(), seq1.value() + 1);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(PredictionManager, ConfirmAlreadyConfirmedReturnsFalse) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::MoveAction move(0, 2, 3);
    auto seqId = setup.prediction.predictAction(setup.playerA, move);
    ASSERT_TRUE(seqId.has_value());

    EXPECT_TRUE(setup.prediction.confirmAction(seqId.value()));
    EXPECT_FALSE(setup.prediction.confirmAction(seqId.value()));
}

TEST(PredictionManager, RejectAlreadyRejectedReturnsFalse) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(2, 2, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    game::MoveAction move(0, 2, 3);
    auto seqId = setup.prediction.predictAction(setup.playerA, move);
    ASSERT_TRUE(seqId.has_value());

    EXPECT_TRUE(setup.prediction.rejectAction(seqId.value()));
    EXPECT_FALSE(setup.prediction.rejectAction(seqId.value()));
}

TEST(PredictionManager, PredictionLifecycleFullFlow) {
    PredictionSetup setup;
    auto unitReg = makeRegistry();
    auto &state = setup.session.mutableState();

    auto warrior = std::make_unique<game::Warrior>(5, 5, unitReg, setup.factionA);
    state.addUnit(std::move(warrior));

    // 1. Predict a move.
    game::MoveAction move1(0, 5, 6);
    auto seq1 = setup.prediction.predictAction(setup.playerA, move1);
    ASSERT_TRUE(seq1.has_value());
    EXPECT_EQ(state.units()[0]->col(), 6);

    // 2. Server confirms.
    EXPECT_TRUE(setup.prediction.confirmAction(seq1.value()));
    EXPECT_EQ(setup.prediction.pendingCount(), 0);

    // 3. Predict another move.
    game::MoveAction move2(0, 5, 7);
    auto seq2 = setup.prediction.predictAction(setup.playerA, move2);
    ASSERT_TRUE(seq2.has_value());
    EXPECT_EQ(state.units()[0]->col(), 7);

    // 4. Server rejects.
    EXPECT_TRUE(setup.prediction.rejectAction(seq2.value()));
    EXPECT_EQ(setup.prediction.pendingCount(), 0);

    // Unit should be back at (5, 6) — the confirmed position.
    EXPECT_EQ(state.units()[0]->col(), 6);
}

} // namespace
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
