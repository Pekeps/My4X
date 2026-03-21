// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/UnitAnimator.h"

#include <gtest/gtest.h>

#include <cmath>
#include <optional>

using engine::UnitAnimator;
using engine::UnitAnimationType;
using engine::UnitId;
using engine::Vec3;

// ── Helper constants ─────────────────────────────────────────────────────────

static constexpr UnitId UNIT_A = 1;
static constexpr UnitId UNIT_B = 2;
static constexpr UnitId UNIT_C = 3;
static constexpr float EPSILON = 0.001F;

static constexpr Vec3 ORIGIN{0.0F, 0.0F, 0.0F};
static constexpr Vec3 POS_A{1.0F, 0.0F, 0.0F};
static constexpr Vec3 POS_B{3.0F, 0.0F, 2.0F};
static constexpr Vec3 POS_C{0.0F, 0.0F, 4.0F};

// ── Initial state ────────────────────────────────────────────────────────────

TEST(UnitAnimatorTest, InitiallyEmpty) {
    UnitAnimator animator;
    EXPECT_EQ(animator.activeCount(), 0);
    EXPECT_FALSE(animator.isAnimating(UNIT_A));
}

TEST(UnitAnimatorTest, UnknownUnit_ReturnsNullopt) {
    UnitAnimator animator;
    EXPECT_EQ(animator.getVisualPosition(42), std::nullopt);
    EXPECT_EQ(animator.getVisualScale(42), std::nullopt);
}

// ── Move animation ──────────────────────────────────────────────────────────

TEST(UnitAnimatorTest, StartMove_IsAnimating) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, POS_A, POS_B);
    EXPECT_TRUE(animator.isAnimating(UNIT_A));
    EXPECT_EQ(animator.activeCount(), 1);
}

TEST(UnitAnimatorTest, Move_InitialPosition_IsFrom) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, POS_A, POS_B);

    auto pos = animator.getVisualPosition(UNIT_A);
    ASSERT_TRUE(pos.has_value());
    EXPECT_NEAR(pos->x, POS_A.x, EPSILON);
    EXPECT_NEAR(pos->z, POS_A.z, EPSILON);
}

TEST(UnitAnimatorTest, Move_AfterFullDuration_ReachesDestination) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, POS_A, POS_B);

    // Advance well past the duration.
    animator.update(engine::MOVE_ANIM_DURATION + 0.1F);

    auto pos = animator.getVisualPosition(UNIT_A);
    ASSERT_TRUE(pos.has_value());
    EXPECT_NEAR(pos->x, POS_B.x, EPSILON);
    EXPECT_NEAR(pos->z, POS_B.z, EPSILON);
}

TEST(UnitAnimatorTest, Move_MidAnimation_IsBetween) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, ORIGIN, POS_B);

    // Advance to roughly halfway.
    animator.update(engine::MOVE_ANIM_DURATION * 0.5F);

    auto pos = animator.getVisualPosition(UNIT_A);
    ASSERT_TRUE(pos.has_value());

    // Should be between origin and destination on x-axis.
    EXPECT_GT(pos->x, ORIGIN.x);
    EXPECT_LT(pos->x, POS_B.x);
}

TEST(UnitAnimatorTest, Move_CompletesAndStopsAnimating) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, POS_A, POS_B);
    animator.update(engine::MOVE_ANIM_DURATION + 0.1F);

    EXPECT_FALSE(animator.isAnimating(UNIT_A));
}

TEST(UnitAnimatorTest, Move_ScaleRemainsOne) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, POS_A, POS_B);
    animator.update(engine::MOVE_ANIM_DURATION * 0.5F);

    auto scale = animator.getVisualScale(UNIT_A);
    ASSERT_TRUE(scale.has_value());
    EXPECT_NEAR(*scale, 1.0F, EPSILON);
}

// ── Attack animation ────────────────────────────────────────────────────────

TEST(UnitAnimatorTest, StartAttack_IsAnimating) {
    UnitAnimator animator;
    animator.startAttack(UNIT_A, POS_A, POS_B);
    EXPECT_TRUE(animator.isAnimating(UNIT_A));
}

TEST(UnitAnimatorTest, Attack_StartsAtUnitPosition) {
    UnitAnimator animator;
    animator.startAttack(UNIT_A, POS_A, POS_B);

    auto pos = animator.getVisualPosition(UNIT_A);
    ASSERT_TRUE(pos.has_value());
    EXPECT_NEAR(pos->x, POS_A.x, EPSILON);
    EXPECT_NEAR(pos->z, POS_A.z, EPSILON);
}

TEST(UnitAnimatorTest, Attack_AtPeak_LungesForward) {
    UnitAnimator animator;
    animator.startAttack(UNIT_A, ORIGIN, POS_A);

    // Advance to the peak moment.
    animator.update(engine::ATTACK_ANIM_DURATION * engine::ATTACK_LUNGE_PEAK);

    auto pos = animator.getVisualPosition(UNIT_A);
    ASSERT_TRUE(pos.has_value());

    // Should have lunged forward toward target (positive x).
    EXPECT_GT(pos->x, ORIGIN.x);
}

TEST(UnitAnimatorTest, Attack_ReturnsToStartAfterCompletion) {
    UnitAnimator animator;
    animator.startAttack(UNIT_A, POS_A, POS_B);
    animator.update(engine::ATTACK_ANIM_DURATION + 0.1F);

    auto pos = animator.getVisualPosition(UNIT_A);
    ASSERT_TRUE(pos.has_value());

    // After completion, lunge fraction should be 0 -> back at from position.
    EXPECT_NEAR(pos->x, POS_A.x, EPSILON);
    EXPECT_NEAR(pos->z, POS_A.z, EPSILON);
}

TEST(UnitAnimatorTest, Attack_CompletesAndStopsAnimating) {
    UnitAnimator animator;
    animator.startAttack(UNIT_A, POS_A, POS_B);
    animator.update(engine::ATTACK_ANIM_DURATION + 0.1F);

    EXPECT_FALSE(animator.isAnimating(UNIT_A));
}

TEST(UnitAnimatorTest, Attack_ScaleRemainsOne) {
    UnitAnimator animator;
    animator.startAttack(UNIT_A, POS_A, POS_B);
    animator.update(engine::ATTACK_ANIM_DURATION * 0.5F);

    auto scale = animator.getVisualScale(UNIT_A);
    ASSERT_TRUE(scale.has_value());
    EXPECT_NEAR(*scale, 1.0F, EPSILON);
}

// ── Death animation ─────────────────────────────────────────────────────────

TEST(UnitAnimatorTest, StartDeath_IsAnimating) {
    UnitAnimator animator;
    animator.startDeath(UNIT_A, POS_A);
    EXPECT_TRUE(animator.isAnimating(UNIT_A));
}

TEST(UnitAnimatorTest, Death_InitialScale_IsOne) {
    UnitAnimator animator;
    animator.startDeath(UNIT_A, POS_A);

    auto scale = animator.getVisualScale(UNIT_A);
    ASSERT_TRUE(scale.has_value());
    EXPECT_NEAR(*scale, 1.0F, EPSILON);
}

TEST(UnitAnimatorTest, Death_MidAnimation_ScaleShrinks) {
    UnitAnimator animator;
    animator.startDeath(UNIT_A, POS_A);
    animator.update(engine::DEATH_ANIM_DURATION * 0.5F);

    auto scale = animator.getVisualScale(UNIT_A);
    ASSERT_TRUE(scale.has_value());
    EXPECT_GT(*scale, 0.0F);
    EXPECT_LT(*scale, 1.0F);
}

TEST(UnitAnimatorTest, Death_AfterCompletion_ScaleIsZero) {
    UnitAnimator animator;
    animator.startDeath(UNIT_A, POS_A);
    animator.update(engine::DEATH_ANIM_DURATION + 0.1F);

    auto scale = animator.getVisualScale(UNIT_A);
    ASSERT_TRUE(scale.has_value());
    EXPECT_NEAR(*scale, 0.0F, EPSILON);
}

TEST(UnitAnimatorTest, Death_SinksBelowGround) {
    UnitAnimator animator;
    animator.startDeath(UNIT_A, POS_A);
    animator.update(engine::DEATH_ANIM_DURATION * 0.5F);

    auto pos = animator.getVisualPosition(UNIT_A);
    ASSERT_TRUE(pos.has_value());
    EXPECT_LT(pos->y, POS_A.y);
}

TEST(UnitAnimatorTest, Death_CompletesAndStopsAnimating) {
    UnitAnimator animator;
    animator.startDeath(UNIT_A, POS_A);
    animator.update(engine::DEATH_ANIM_DURATION + 0.1F);

    EXPECT_FALSE(animator.isAnimating(UNIT_A));
}

// ── Multiple units ──────────────────────────────────────────────────────────

TEST(UnitAnimatorTest, MultipleUnits_TrackedIndependently) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, ORIGIN, POS_A);
    animator.startAttack(UNIT_B, POS_A, POS_B);
    animator.startDeath(UNIT_C, POS_C);

    EXPECT_EQ(animator.activeCount(), 3);
    EXPECT_TRUE(animator.isAnimating(UNIT_A));
    EXPECT_TRUE(animator.isAnimating(UNIT_B));
    EXPECT_TRUE(animator.isAnimating(UNIT_C));
}

TEST(UnitAnimatorTest, MultipleUnits_FinishIndependently) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, ORIGIN, POS_A);
    animator.startDeath(UNIT_B, POS_B);

    // Move finishes sooner than death if durations differ.
    float maxDuration = std::max(engine::MOVE_ANIM_DURATION, engine::DEATH_ANIM_DURATION);
    animator.update(maxDuration + 0.1F);

    EXPECT_FALSE(animator.isAnimating(UNIT_A));
    EXPECT_FALSE(animator.isAnimating(UNIT_B));
    EXPECT_EQ(animator.activeCount(), 0);
}

// ── Override: starting a new animation replaces the old one ─────────────────

TEST(UnitAnimatorTest, StartingNewAnimation_ReplacesOld) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, ORIGIN, POS_A);
    animator.update(engine::MOVE_ANIM_DURATION * 0.5F);

    // Start a new death animation for the same unit.
    animator.startDeath(UNIT_A, POS_B);

    // Should be animating the new death, scale should be 1 (just started).
    EXPECT_TRUE(animator.isAnimating(UNIT_A));
    auto scale = animator.getVisualScale(UNIT_A);
    ASSERT_TRUE(scale.has_value());
    EXPECT_NEAR(*scale, 1.0F, EPSILON);
}

// ── removeFinished ──────────────────────────────────────────────────────────

TEST(UnitAnimatorTest, RemoveFinished_CleansUpCompletedAnimations) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, ORIGIN, POS_A);
    animator.update(engine::MOVE_ANIM_DURATION + 0.1F);

    // Animation is finished but entry still exists.
    EXPECT_FALSE(animator.isAnimating(UNIT_A));
    EXPECT_TRUE(animator.getVisualPosition(UNIT_A).has_value());

    animator.removeFinished();

    // Now the entry should be gone entirely.
    EXPECT_EQ(animator.getVisualPosition(UNIT_A), std::nullopt);
    EXPECT_EQ(animator.getVisualScale(UNIT_A), std::nullopt);
}

TEST(UnitAnimatorTest, RemoveFinished_KeepsActiveAnimations) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, ORIGIN, POS_A);
    animator.startDeath(UNIT_B, POS_B);

    // Advance only partially.
    animator.update(0.01F);

    animator.removeFinished();

    // Both should still be tracked.
    EXPECT_TRUE(animator.getVisualPosition(UNIT_A).has_value());
    EXPECT_TRUE(animator.getVisualPosition(UNIT_B).has_value());
}

// ── Frame-rate independence ─────────────────────────────────────────────────

TEST(UnitAnimatorTest, FrameRateIndependence_SameResult) {
    // Simulate at two different frame rates, final position should be the same.
    UnitAnimator fast;
    fast.startMove(UNIT_A, ORIGIN, POS_B);

    UnitAnimator slow;
    slow.startMove(UNIT_A, ORIGIN, POS_B);

    // "fast" does 40 steps of 0.01s = 0.4s total.
    for (int i = 0; i < 40; ++i) {
        fast.update(0.01F);
    }

    // "slow" does 4 steps of 0.1s = 0.4s total.
    for (int i = 0; i < 4; ++i) {
        slow.update(0.1F);
    }

    auto fastPos = fast.getVisualPosition(UNIT_A);
    auto slowPos = slow.getVisualPosition(UNIT_A);

    ASSERT_TRUE(fastPos.has_value());
    ASSERT_TRUE(slowPos.has_value());

    // Both should reach the destination (or very close).
    EXPECT_NEAR(fastPos->x, POS_B.x, EPSILON);
    EXPECT_NEAR(fastPos->z, POS_B.z, EPSILON);
    EXPECT_NEAR(slowPos->x, POS_B.x, EPSILON);
    EXPECT_NEAR(slowPos->z, POS_B.z, EPSILON);
}

// ── Zero delta time ─────────────────────────────────────────────────────────

TEST(UnitAnimatorTest, ZeroDeltaTime_NoProgress) {
    UnitAnimator animator;
    animator.startMove(UNIT_A, POS_A, POS_B);
    animator.update(0.0F);

    auto pos = animator.getVisualPosition(UNIT_A);
    ASSERT_TRUE(pos.has_value());
    EXPECT_NEAR(pos->x, POS_A.x, EPSILON);
    EXPECT_NEAR(pos->z, POS_A.z, EPSILON);
    EXPECT_TRUE(animator.isAnimating(UNIT_A));
}

// ── Move interpolation uses smooth-step (ease-in-out) ───────────────────────

TEST(UnitAnimatorTest, Move_UsesEaseInOut) {
    UnitAnimator animator;
    // Move along x-axis only for easy checking.
    Vec3 from{0.0F, 0.0F, 0.0F};
    Vec3 to{10.0F, 0.0F, 0.0F};
    animator.startMove(UNIT_A, from, to);

    // At t=0.25 (quarter), smooth-step should give a non-linear value.
    animator.update(engine::MOVE_ANIM_DURATION * 0.25F);
    auto pos = animator.getVisualPosition(UNIT_A);
    ASSERT_TRUE(pos.has_value());

    // Linear would give 2.5. Smooth-step at 0.25 = 0.15625, so 1.5625.
    // Verify it's not linear (less than the linear midpoint).
    float linearExpected = 2.5F;
    EXPECT_LT(pos->x, linearExpected);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
