// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/CameraCinematic.h"

#include <gtest/gtest.h>

#include <cmath>

using engine::CameraCinematic;
using engine::CameraSnapshot;
using engine::CinematicPhase;

// ── Helper to create a default camera snapshot ──────────────────────────────

static CameraSnapshot makeSnapshot(float x, float z, float distance, float angle) {
    CameraSnapshot snap;
    snap.target = {x, 0.0F, z};
    snap.distance = distance;
    snap.angle = angle;
    return snap;
}

// ── Basic lifecycle tests ───────────────────────────────────────────────────

TEST(CameraCinematicTest, InitiallyIdle) {
    CameraCinematic cine;
    EXPECT_FALSE(cine.isActive());
    EXPECT_EQ(cine.phase(), CinematicPhase::Idle);
}

TEST(CameraCinematicTest, TriggerStartsPanPhase) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    EXPECT_TRUE(cine.isActive());
    EXPECT_EQ(cine.phase(), CinematicPhase::PanToTarget);
}

TEST(CameraCinematicTest, UpdateDoesNothingWhenIdle) {
    CameraCinematic cine;
    // Should not crash or change state.
    cine.update(1.0F);
    EXPECT_FALSE(cine.isActive());
    EXPECT_EQ(cine.phase(), CinematicPhase::Idle);
}

// ── Phase transitions ───────────────────────────────────────────────────────

TEST(CameraCinematicTest, PanTransitionsToHold) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    // Advance past the pan duration (0.6s).
    cine.update(0.7F);
    EXPECT_TRUE(cine.isActive());
    EXPECT_EQ(cine.phase(), CinematicPhase::Hold);
}

TEST(CameraCinematicTest, HoldTransitionsToReturn) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    // Advance past pan (0.6s) + hold (0.8s).
    cine.update(0.7F);  // -> Hold
    cine.update(0.9F);  // -> ReturnToOrigin
    EXPECT_TRUE(cine.isActive());
    EXPECT_EQ(cine.phase(), CinematicPhase::ReturnToOrigin);
}

TEST(CameraCinematicTest, ReturnTransitionsToIdle) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    // Advance through all phases: pan(0.6) + hold(0.8) + return(0.5).
    cine.update(0.7F);  // -> Hold
    cine.update(0.9F);  // -> ReturnToOrigin
    cine.update(0.6F);  // -> Idle
    EXPECT_FALSE(cine.isActive());
    EXPECT_EQ(cine.phase(), CinematicPhase::Idle);
}

TEST(CameraCinematicTest, FullSequenceRestoresOriginSnapshot) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 15.0F, 30.0F, 1.2F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    // Run through all phases.
    cine.update(0.7F);
    cine.update(0.9F);
    cine.update(0.6F);

    auto result = cine.currentSnapshot();
    EXPECT_NEAR(result.target.x, 10.0F, 0.01F);
    EXPECT_NEAR(result.target.z, 15.0F, 0.01F);
    EXPECT_NEAR(result.distance, 30.0F, 0.01F);
    EXPECT_NEAR(result.angle, 1.2F, 0.01F);
}

// ── Skip behavior ───────────────────────────────────────────────────────────

TEST(CameraCinematicTest, SkipDuringPanGoesToIdle) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    cine.update(0.2F); // partway through pan
    cine.skip();

    EXPECT_FALSE(cine.isActive());
    EXPECT_EQ(cine.phase(), CinematicPhase::Idle);
}

TEST(CameraCinematicTest, SkipDuringHoldGoesToIdle) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    cine.update(0.7F); // -> Hold
    cine.update(0.3F); // partway through hold
    cine.skip();

    EXPECT_FALSE(cine.isActive());
    EXPECT_EQ(cine.phase(), CinematicPhase::Idle);
}

TEST(CameraCinematicTest, SkipDuringReturnGoesToIdle) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    cine.update(0.7F); // -> Hold
    cine.update(0.9F); // -> ReturnToOrigin
    cine.update(0.2F); // partway through return
    cine.skip();

    EXPECT_FALSE(cine.isActive());
    EXPECT_EQ(cine.phase(), CinematicPhase::Idle);
}

TEST(CameraCinematicTest, SkipRestoresOriginSnapshot) {
    CameraCinematic cine;
    auto origin = makeSnapshot(5.0F, 8.0F, 25.0F, 0.5F);
    Vector3 combatPos = {30.0F, 0.0F, 30.0F};
    cine.triggerCombat(combatPos, origin);

    cine.update(0.3F); // partway through pan
    cine.skip();

    auto result = cine.currentSnapshot();
    EXPECT_NEAR(result.target.x, 5.0F, 0.01F);
    EXPECT_NEAR(result.target.z, 8.0F, 0.01F);
    EXPECT_NEAR(result.distance, 25.0F, 0.01F);
    EXPECT_NEAR(result.angle, 0.5F, 0.01F);
}

TEST(CameraCinematicTest, SkipWhenIdleIsNoOp) {
    CameraCinematic cine;
    cine.skip(); // Should not crash.
    EXPECT_FALSE(cine.isActive());
}

// ── Interpolation behavior ──────────────────────────────────────────────────

TEST(CameraCinematicTest, MidPanSnapshotIsInterpolated) {
    CameraCinematic cine;
    auto origin = makeSnapshot(0.0F, 0.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    // Advance to roughly halfway through pan (0.3 / 0.6 = 0.5).
    cine.update(0.3F);
    auto snap = cine.currentSnapshot();

    // The target should be somewhere between origin (0,0) and combat (20,20).
    EXPECT_GT(snap.target.x, 0.0F);
    EXPECT_LT(snap.target.x, 20.0F);
    EXPECT_GT(snap.target.z, 0.0F);
    EXPECT_LT(snap.target.z, 20.0F);

    // Distance should be between origin (30) and combat zoom (12).
    EXPECT_GT(snap.distance, 12.0F);
    EXPECT_LT(snap.distance, 30.0F);
}

TEST(CameraCinematicTest, HoldPhaseSnapshotMatchesCombat) {
    CameraCinematic cine;
    auto origin = makeSnapshot(0.0F, 0.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    cine.update(0.7F); // past pan -> Hold
    auto snap = cine.currentSnapshot();

    // During hold, snapshot should match combat position.
    EXPECT_NEAR(snap.target.x, 20.0F, 0.01F);
    EXPECT_NEAR(snap.target.z, 20.0F, 0.01F);
    EXPECT_NEAR(snap.distance, 12.0F, 0.01F);
}

// ── Camera3D output ─────────────────────────────────────────────────────────

TEST(CameraCinematicTest, GetCameraReturnsSensibleCamera3D) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    Camera3D cam = cine.getCamera();
    // Camera should be positioned above the map (positive Y).
    EXPECT_GT(cam.position.y, 0.0F);
    // FOV should be 45 degrees.
    EXPECT_NEAR(cam.fovy, 45.0F, 0.01F);
    // Should be perspective projection.
    EXPECT_EQ(cam.projection, CAMERA_PERSPECTIVE);
}

// ── Frame-rate independence ─────────────────────────────────────────────────

TEST(CameraCinematicTest, SmallTimestepsProduceSameResult) {
    // One big update vs many small ones should produce a similar snapshot
    // at the same elapsed time (0.5s, within the pan phase).
    CameraCinematic cine1;
    CameraCinematic cine2;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};

    // Big single update (0.5s into pan phase, which lasts 0.6s).
    cine1.triggerCombat(combatPos, origin);
    cine1.update(0.5F);

    // Many small updates totaling the same time.
    cine2.triggerCombat(combatPos, origin);
    for (int i = 0; i < 50; ++i) {
        cine2.update(0.01F);
    }

    // Both should still be in PanToTarget phase.
    EXPECT_EQ(cine1.phase(), CinematicPhase::PanToTarget);
    EXPECT_EQ(cine2.phase(), CinematicPhase::PanToTarget);

    // The interpolated snapshots should be very close.
    auto snap1 = cine1.currentSnapshot();
    auto snap2 = cine2.currentSnapshot();
    EXPECT_NEAR(snap1.target.x, snap2.target.x, 0.1F);
    EXPECT_NEAR(snap1.target.z, snap2.target.z, 0.1F);
    EXPECT_NEAR(snap1.distance, snap2.distance, 0.1F);
}

// ── Re-trigger during active cinematic ──────────────────────────────────────

TEST(CameraCinematicTest, RetriggerRestartsFromCurrentOrigin) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 0.0F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    cine.update(0.3F); // partway through pan

    // Re-trigger with a new position and new origin.
    auto newOrigin = makeSnapshot(5.0F, 5.0F, 25.0F, 1.0F);
    Vector3 newCombatPos = {30.0F, 0.0F, 30.0F};
    cine.triggerCombat(newCombatPos, newOrigin);

    EXPECT_TRUE(cine.isActive());
    EXPECT_EQ(cine.phase(), CinematicPhase::PanToTarget);
}

// ── Angle preservation ──────────────────────────────────────────────────────

TEST(CameraCinematicTest, CombatCameraPreservesAngle) {
    CameraCinematic cine;
    auto origin = makeSnapshot(10.0F, 10.0F, 30.0F, 2.1F);
    Vector3 combatPos = {20.0F, 0.0F, 20.0F};
    cine.triggerCombat(combatPos, origin);

    // Advance to hold phase — angle should match origin.
    cine.update(0.7F);
    auto snap = cine.currentSnapshot();
    EXPECT_NEAR(snap.angle, 2.1F, 0.01F);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
