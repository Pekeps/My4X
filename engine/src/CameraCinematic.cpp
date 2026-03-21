#include "engine/CameraCinematic.h"

#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace engine {

void CameraCinematic::triggerCombat(Vector3 combatPosition, const CameraSnapshot &currentCamera) {
    origin_ = currentCamera;
    combat_.target = {.x = combatPosition.x, .y = TARGET_Y, .z = combatPosition.z};
    combat_.distance = COMBAT_ZOOM_DISTANCE;
    combat_.angle = currentCamera.angle; // Keep the same rotation angle.
    current_ = origin_;
    phase_ = CinematicPhase::PanToTarget;
    elapsed_ = 0.0F;
}

void CameraCinematic::update(float deltaTime) {
    if (phase_ == CinematicPhase::Idle) {
        return;
    }

    elapsed_ += deltaTime;

    switch (phase_) {
    case CinematicPhase::PanToTarget: {
        float t = std::clamp(elapsed_ / PAN_DURATION, 0.0F, 1.0F);
        current_ = lerpSnapshot(origin_, combat_, smoothStep(t));
        if (elapsed_ >= PAN_DURATION) {
            current_ = combat_;
            phase_ = CinematicPhase::Hold;
            elapsed_ = 0.0F;
        }
        break;
    }
    case CinematicPhase::Hold: {
        current_ = combat_;
        if (elapsed_ >= HOLD_DURATION) {
            phase_ = CinematicPhase::ReturnToOrigin;
            elapsed_ = 0.0F;
        }
        break;
    }
    case CinematicPhase::ReturnToOrigin: {
        float t = std::clamp(elapsed_ / RETURN_DURATION, 0.0F, 1.0F);
        current_ = lerpSnapshot(combat_, origin_, smoothStep(t));
        if (elapsed_ >= RETURN_DURATION) {
            current_ = origin_;
            phase_ = CinematicPhase::Idle;
            elapsed_ = 0.0F;
        }
        break;
    }
    case CinematicPhase::Idle:
        break;
    }
}

void CameraCinematic::skip() {
    if (phase_ == CinematicPhase::Idle) {
        return;
    }
    current_ = origin_;
    phase_ = CinematicPhase::Idle;
    elapsed_ = 0.0F;
}

bool CameraCinematic::isActive() const { return phase_ != CinematicPhase::Idle; }

CinematicPhase CameraCinematic::phase() const { return phase_; }

Camera3D CameraCinematic::getCamera() const { return snapshotToCamera3D(current_); }

CameraSnapshot CameraCinematic::currentSnapshot() const { return current_; }

float CameraCinematic::smoothStep(float t) {
    // Hermite smoothstep: 3t^2 - 2t^3
    static constexpr float SMOOTH_COEFF_A = 3.0F;
    static constexpr float SMOOTH_COEFF_B = 2.0F;
    return t * t * (SMOOTH_COEFF_A - SMOOTH_COEFF_B * t);
}

Camera3D CameraCinematic::snapshotToCamera3D(const CameraSnapshot &snap) {
    // Compute pitch based on zoom distance (matching Camera::raw() logic).
    float zoomFactor = (snap.distance - MIN_PITCH_AT_DISTANCE) / (MAX_DISTANCE - MIN_PITCH_AT_DISTANCE);
    zoomFactor = std::clamp(zoomFactor, 0.0F, 1.0F);
    float pitch = Lerp(MIN_PITCH, MAX_PITCH, zoomFactor);

    Vector3 position = {
        .x = snap.target.x + (snap.distance * cosf(pitch) * sinf(snap.angle)),
        .y = snap.target.y + (snap.distance * sinf(pitch)),
        .z = snap.target.z + (snap.distance * cosf(pitch) * cosf(snap.angle)),
    };

    Camera3D cam = {};
    cam.position = position;
    cam.target = snap.target;
    cam.up = {.x = 0.0F, .y = 1.0F, .z = 0.0F};
    cam.fovy = CAMERA_FOV;
    cam.projection = CAMERA_PERSPECTIVE;

    return cam;
}

CameraSnapshot CameraCinematic::lerpSnapshot(const CameraSnapshot &a, const CameraSnapshot &b, float t) {
    CameraSnapshot result;
    result.target = Vector3Lerp(a.target, b.target, t);
    result.distance = Lerp(a.distance, b.distance, t);
    result.angle = Lerp(a.angle, b.angle, t);
    return result;
}

} // namespace engine
