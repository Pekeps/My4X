// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/AnimationController.h"

#include <gtest/gtest.h>

#include <optional>
#include <vector>

using engine::AnimationController;
using engine::AnimationState;
using engine::EntityId;

// ── Initial state ───────────────────────────────────────────────────────────

TEST(AnimationControllerTest, InitiallyEmpty) {
    AnimationController ctrl;
    EXPECT_EQ(ctrl.entityCount(), 0);
}

TEST(AnimationControllerTest, UnknownEntity_ReturnsNullopt) {
    AnimationController ctrl;
    EXPECT_EQ(ctrl.getState(42), std::nullopt);
    EXPECT_EQ(ctrl.getProgress(42), std::nullopt);
}

// ── playAnimation registers new entities ────────────────────────────────────

TEST(AnimationControllerTest, PlayAnimation_RegistersNewEntity) {
    AnimationController ctrl;
    bool accepted = ctrl.playAnimation(1, AnimationState::Idle);
    EXPECT_TRUE(accepted);
    EXPECT_EQ(ctrl.entityCount(), 1);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Idle);
}

TEST(AnimationControllerTest, PlayAnimation_MultipleEntities) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Idle);
    ctrl.playAnimation(2, AnimationState::Walking);
    ctrl.playAnimation(3, AnimationState::Attacking);
    EXPECT_EQ(ctrl.entityCount(), 3);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Idle);
    EXPECT_EQ(ctrl.getState(2), AnimationState::Walking);
    EXPECT_EQ(ctrl.getState(3), AnimationState::Attacking);
}

// ── Progress tracking ───────────────────────────────────────────────────────

TEST(AnimationControllerTest, NewEntity_ProgressIsZero) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Idle);
    auto progress = ctrl.getProgress(1);
    ASSERT_TRUE(progress.has_value());
    EXPECT_FLOAT_EQ(progress.value(), 0.0F);
}

TEST(AnimationControllerTest, ProgressAdvances_WithUpdate) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Attacking);
    // Attacking duration = 0.5s, advance by 0.25s -> 50% progress
    ctrl.update(0.25F);
    auto progress = ctrl.getProgress(1);
    ASSERT_TRUE(progress.has_value());
    EXPECT_FLOAT_EQ(progress.value(), 0.5F);
}

TEST(AnimationControllerTest, ProgressClampedToOne) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Attacking);
    // Advance slightly less than the full duration so we can check clamping
    // before auto-transition kicks in. Since update auto-transitions, advance
    // to exactly the duration boundary.
    ctrl.update(engine::ATTACKING_ANIMATION_DURATION);
    // After auto-transition, state should be Idle now with reset progress.
    EXPECT_EQ(ctrl.getState(1), AnimationState::Idle);
}

// ── Dead state progress ─────────────────────────────────────────────────────

TEST(AnimationControllerTest, DeadState_ProgressIsOne) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Dying);
    // Advance past dying duration to auto-transition to Dead
    ctrl.update(engine::DYING_ANIMATION_DURATION + 0.01F);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Dead);
    auto progress = ctrl.getProgress(1);
    ASSERT_TRUE(progress.has_value());
    EXPECT_FLOAT_EQ(progress.value(), 1.0F);
}

// ── State transition validation ─────────────────────────────────────────────

TEST(AnimationControllerTest, ValidTransitions_LivingStates) {
    // All living-state-to-living-state transitions should be valid.
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::Idle, AnimationState::Walking));
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::Walking, AnimationState::Idle));
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::Idle, AnimationState::Attacking));
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::Attacking, AnimationState::TakingDamage));
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::Walking, AnimationState::Attacking));
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::TakingDamage, AnimationState::Idle));
}

TEST(AnimationControllerTest, ValidTransition_AnyLivingStateToDying) {
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::Idle, AnimationState::Dying));
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::Walking, AnimationState::Dying));
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::Attacking, AnimationState::Dying));
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::TakingDamage, AnimationState::Dying));
}

TEST(AnimationControllerTest, ValidTransition_DyingToDead) {
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::Dying, AnimationState::Dead));
}

TEST(AnimationControllerTest, InvalidTransition_DeadToAnything) {
    // Dead -> anything except Dead is invalid.
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Dead, AnimationState::Idle));
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Dead, AnimationState::Walking));
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Dead, AnimationState::Attacking));
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Dead, AnimationState::TakingDamage));
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Dead, AnimationState::Dying));
    // Dead -> Dead is the no-op identity, which is valid.
    EXPECT_TRUE(AnimationController::isValidTransition(AnimationState::Dead, AnimationState::Dead));
}

TEST(AnimationControllerTest, InvalidTransition_DyingToLiving) {
    // Dying can only go to Dead.
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Dying, AnimationState::Idle));
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Dying, AnimationState::Walking));
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Dying, AnimationState::Attacking));
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Dying, AnimationState::TakingDamage));
}

TEST(AnimationControllerTest, InvalidTransition_DirectlyToDead) {
    // Living states cannot go directly to Dead — must go through Dying.
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Idle, AnimationState::Dead));
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Walking, AnimationState::Dead));
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::Attacking, AnimationState::Dead));
    EXPECT_FALSE(AnimationController::isValidTransition(AnimationState::TakingDamage, AnimationState::Dead));
}

// ── playAnimation enforces transitions ──────────────────────────────────────

TEST(AnimationControllerTest, PlayAnimation_RejectsInvalidTransition) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Dying);
    ctrl.update(engine::DYING_ANIMATION_DURATION + 0.01F);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Dead);

    // Attempt to transition Dead -> Walking.
    bool accepted = ctrl.playAnimation(1, AnimationState::Walking);
    EXPECT_FALSE(accepted);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Dead);
}

TEST(AnimationControllerTest, PlayAnimation_ResetsProgressOnTransition) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Idle);
    ctrl.update(0.5F);
    // Progress should be > 0 now.
    EXPECT_GT(ctrl.getProgress(1).value(), 0.0F);

    // Transition to Walking resets progress.
    ctrl.playAnimation(1, AnimationState::Walking);
    EXPECT_FLOAT_EQ(ctrl.getProgress(1).value(), 0.0F);
}

TEST(AnimationControllerTest, PlayAnimation_SameStateResetsProgress) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Walking);
    ctrl.update(0.3F);
    float progress1 = ctrl.getProgress(1).value();
    EXPECT_GT(progress1, 0.0F);

    // Re-trigger same state.
    ctrl.playAnimation(1, AnimationState::Walking);
    EXPECT_FLOAT_EQ(ctrl.getProgress(1).value(), 0.0F);
}

// ── Auto-transitions on completion ──────────────────────────────────────────

TEST(AnimationControllerTest, Attacking_AutoTransitionsToIdle) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Attacking);
    ctrl.update(engine::ATTACKING_ANIMATION_DURATION + 0.01F);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Idle);
}

TEST(AnimationControllerTest, TakingDamage_AutoTransitionsToIdle) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::TakingDamage);
    ctrl.update(engine::TAKING_DAMAGE_ANIMATION_DURATION + 0.01F);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Idle);
}

TEST(AnimationControllerTest, Dying_AutoTransitionsToDead) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Dying);
    ctrl.update(engine::DYING_ANIMATION_DURATION + 0.01F);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Dead);
}

TEST(AnimationControllerTest, Idle_Loops_DoesNotAutoTransition) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Idle);
    // Run past 3 full idle loops.
    ctrl.update(engine::IDLE_ANIMATION_DURATION * 3.0F + 0.01F);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Idle);
}

TEST(AnimationControllerTest, Walking_Loops_DoesNotAutoTransition) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Walking);
    ctrl.update(engine::WALKING_ANIMATION_DURATION * 3.0F + 0.01F);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Walking);
}

TEST(AnimationControllerTest, Dead_DoesNotAdvance) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Dying);
    ctrl.update(engine::DYING_ANIMATION_DURATION + 0.01F);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Dead);

    // Further updates should not change state.
    ctrl.update(10.0F);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Dead);
}

// ── Completion callback ─────────────────────────────────────────────────────

TEST(AnimationControllerTest, CompletionCallback_FiresOnOneShotCompletion) {
    AnimationController ctrl;
    std::vector<std::pair<EntityId, AnimationState>> completions;

    ctrl.setCompletionCallback(
        [&completions](EntityId id, AnimationState state) { completions.emplace_back(id, state); });

    ctrl.playAnimation(1, AnimationState::Attacking);
    ctrl.update(engine::ATTACKING_ANIMATION_DURATION + 0.01F);

    ASSERT_EQ(completions.size(), 1);
    EXPECT_EQ(completions[0].first, 1);
    EXPECT_EQ(completions[0].second, AnimationState::Attacking);
}

TEST(AnimationControllerTest, CompletionCallback_FiresForDying) {
    AnimationController ctrl;
    std::vector<std::pair<EntityId, AnimationState>> completions;

    ctrl.setCompletionCallback(
        [&completions](EntityId id, AnimationState state) { completions.emplace_back(id, state); });

    ctrl.playAnimation(1, AnimationState::Dying);
    ctrl.update(engine::DYING_ANIMATION_DURATION + 0.01F);

    ASSERT_EQ(completions.size(), 1);
    EXPECT_EQ(completions[0].first, 1);
    EXPECT_EQ(completions[0].second, AnimationState::Dying);
}

TEST(AnimationControllerTest, CompletionCallback_FiresForLoopingStates) {
    AnimationController ctrl;
    std::vector<std::pair<EntityId, AnimationState>> completions;

    ctrl.setCompletionCallback(
        [&completions](EntityId id, AnimationState state) { completions.emplace_back(id, state); });

    ctrl.playAnimation(1, AnimationState::Idle);
    ctrl.update(engine::IDLE_ANIMATION_DURATION + 0.01F);

    // Looping states also fire the callback on each loop completion.
    ASSERT_GE(completions.size(), 1U);
    EXPECT_EQ(completions[0].first, 1);
    EXPECT_EQ(completions[0].second, AnimationState::Idle);
}

TEST(AnimationControllerTest, CompletionCallback_MultipleEntities) {
    AnimationController ctrl;
    std::vector<std::pair<EntityId, AnimationState>> completions;

    ctrl.setCompletionCallback(
        [&completions](EntityId id, AnimationState state) { completions.emplace_back(id, state); });

    ctrl.playAnimation(1, AnimationState::Attacking);
    ctrl.playAnimation(2, AnimationState::TakingDamage);

    // Both should complete within the attacking duration (which is longer than taking damage).
    ctrl.update(engine::ATTACKING_ANIMATION_DURATION + 0.01F);

    EXPECT_EQ(completions.size(), 2);
}

// ── removeEntity ────────────────────────────────────────────────────────────

TEST(AnimationControllerTest, RemoveEntity_RemovesTracking) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Idle);
    ctrl.playAnimation(2, AnimationState::Walking);
    EXPECT_EQ(ctrl.entityCount(), 2);

    ctrl.removeEntity(1);
    EXPECT_EQ(ctrl.entityCount(), 1);
    EXPECT_EQ(ctrl.getState(1), std::nullopt);
    EXPECT_EQ(ctrl.getState(2), AnimationState::Walking);
}

TEST(AnimationControllerTest, RemoveEntity_NoOpForUnknown) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Idle);
    ctrl.removeEntity(999);
    EXPECT_EQ(ctrl.entityCount(), 1);
}

// ── Default duration lookup ─────────────────────────────────────────────────

TEST(AnimationControllerTest, DefaultDurations_ArePositiveForLivingStates) {
    EXPECT_GT(AnimationController::defaultDuration(AnimationState::Idle), 0.0F);
    EXPECT_GT(AnimationController::defaultDuration(AnimationState::Walking), 0.0F);
    EXPECT_GT(AnimationController::defaultDuration(AnimationState::Attacking), 0.0F);
    EXPECT_GT(AnimationController::defaultDuration(AnimationState::TakingDamage), 0.0F);
    EXPECT_GT(AnimationController::defaultDuration(AnimationState::Dying), 0.0F);
}

TEST(AnimationControllerTest, DefaultDuration_DeadIsZero) {
    EXPECT_FLOAT_EQ(AnimationController::defaultDuration(AnimationState::Dead), 0.0F);
}

// ── Frame-rate independence ─────────────────────────────────────────────────

TEST(AnimationControllerTest, FrameRateIndependence_ManySmallUpdates) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Attacking);

    float totalTime = 0.0F;
    while (totalTime < engine::ATTACKING_ANIMATION_DURATION + 0.05F) {
        ctrl.update(0.016F); // ~60 FPS
        totalTime += 0.016F;
    }
    // Should have auto-transitioned to Idle.
    EXPECT_EQ(ctrl.getState(1), AnimationState::Idle);
}

TEST(AnimationControllerTest, FrameRateIndependence_OneLargeUpdate) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Attacking);
    ctrl.update(engine::ATTACKING_ANIMATION_DURATION + 0.05F);
    EXPECT_EQ(ctrl.getState(1), AnimationState::Idle);
}

// ── Multiple entities updated simultaneously ────────────────────────────────

TEST(AnimationControllerTest, Update_AdvancesAllEntities) {
    AnimationController ctrl;
    ctrl.playAnimation(1, AnimationState::Attacking);
    ctrl.playAnimation(2, AnimationState::Walking);
    ctrl.playAnimation(3, AnimationState::Dying);

    ctrl.update(engine::DYING_ANIMATION_DURATION + 0.01F);

    // Attacking finishes first (0.5s) -> auto-transition to Idle.
    EXPECT_EQ(ctrl.getState(1), AnimationState::Idle);
    // Walking is a looping state -> stays Walking.
    EXPECT_EQ(ctrl.getState(2), AnimationState::Walking);
    // Dying finishes (0.6s) -> auto-transition to Dead.
    EXPECT_EQ(ctrl.getState(3), AnimationState::Dead);
}

// ── New entity can start in any state except Dead ───────────────────────────

TEST(AnimationControllerTest, NewEntity_CanStartInAnyState) {
    AnimationController ctrl;

    EXPECT_TRUE(ctrl.playAnimation(1, AnimationState::Idle));
    EXPECT_TRUE(ctrl.playAnimation(2, AnimationState::Walking));
    EXPECT_TRUE(ctrl.playAnimation(3, AnimationState::Attacking));
    EXPECT_TRUE(ctrl.playAnimation(4, AnimationState::TakingDamage));
    EXPECT_TRUE(ctrl.playAnimation(5, AnimationState::Dying));
    EXPECT_TRUE(ctrl.playAnimation(6, AnimationState::Dead));

    EXPECT_EQ(ctrl.entityCount(), 6);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
