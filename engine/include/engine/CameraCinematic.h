#pragma once

#include "raylib.h"

#include <cstdint>

namespace engine {

/// Phases of the combat camera cinematic.
enum class CinematicPhase : std::uint8_t {
    Idle,          ///< No cinematic active.
    PanToTarget,   ///< Smoothly moving toward the combat position.
    Hold,          ///< Holding on the combat position.
    ReturnToOrigin ///< Smoothly returning to the original camera position.
};

/// Camera state captured for interpolation: target point, distance, and angle.
struct CameraSnapshot {
    Vector3 target = {.x = 0.0F, .y = 0.0F, .z = 0.0F};
    float distance = 0.0F;
    float angle = 0.0F;
};

/// Manages a cinematic camera sequence for combat events.
///
/// When combat occurs, the camera smoothly pans to the combat location, zooms
/// in, holds briefly, then smoothly returns to the player's original position.
///
/// Usage:
///   1. Call triggerCombat(position, currentCamera) to start the cinematic.
///   2. Call update(deltaTime) each frame.
///   3. While isActive(), use getCamera() for the camera state and disable
///      player camera input.
///   4. Call skip() if the player wants to skip the cinematic.
class CameraCinematic {
  public:
    /// Start a combat cinematic toward the given world position.
    /// Captures the current camera state for later restoration.
    void triggerCombat(Vector3 combatPosition, const CameraSnapshot &currentCamera);

    /// Advance the cinematic by delta-time seconds.
    void update(float deltaTime);

    /// Skip the cinematic and immediately restore the original camera.
    void skip();

    /// Returns true if a cinematic is currently playing.
    [[nodiscard]] bool isActive() const;

    /// Returns the current cinematic phase (for testing).
    [[nodiscard]] CinematicPhase phase() const;

    /// Returns the current cinematic camera state as a Camera3D.
    [[nodiscard]] Camera3D getCamera() const;

    /// Returns the current camera snapshot (target, distance, angle) for
    /// restoring the player camera after cinematic ends.
    [[nodiscard]] CameraSnapshot currentSnapshot() const;

  private:
    /// Duration of the pan-to-target phase in seconds.
    static constexpr float PAN_DURATION = 0.6F;

    /// Duration of the hold phase in seconds.
    static constexpr float HOLD_DURATION = 0.8F;

    /// Duration of the return-to-origin phase in seconds.
    static constexpr float RETURN_DURATION = 0.5F;

    /// Distance from the combat target when zoomed in.
    static constexpr float COMBAT_ZOOM_DISTANCE = 12.0F;

    /// Camera field-of-view angle in degrees.
    static constexpr float CAMERA_FOV = 45.0F;

    /// Y component of camera target (map plane).
    static constexpr float TARGET_Y = 0.0F;

    /// Pitch range constants (matching Camera.cpp behavior).
    static constexpr float MIN_PITCH_AT_DISTANCE = 20.0F;
    static constexpr float MIN_PITCH = 0.6F;
    static constexpr float MAX_PITCH = 1.55F;
    static constexpr float MAX_DISTANCE = 60.0F;

    /// Smooth-step interpolation: ease in and out.
    [[nodiscard]] static float smoothStep(float t);

    /// Compute Camera3D from a snapshot.
    [[nodiscard]] static Camera3D snapshotToCamera3D(const CameraSnapshot &snap);

    /// Lerp between two snapshots at fraction t.
    [[nodiscard]] static CameraSnapshot lerpSnapshot(const CameraSnapshot &a, const CameraSnapshot &b, float t);

    CinematicPhase phase_ = CinematicPhase::Idle;
    float elapsed_ = 0.0F;

    CameraSnapshot origin_;  ///< Where the camera was before the cinematic.
    CameraSnapshot combat_;  ///< The combat camera destination.
    CameraSnapshot current_; ///< Interpolated current state.
};

} // namespace engine
