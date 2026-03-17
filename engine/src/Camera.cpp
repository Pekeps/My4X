#include "engine/Camera.h"

#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace engine {

// Camera defaults: positioned above and behind, looking down at origin.
const float DEFAULT_DISTANCE = 20.0F;
const float DEFAULT_PITCH = 1.0F; // ~57 degrees from horizontal
const float DEFAULT_ANGLE = 0.0F;
const float DEFAULT_FOV = 45.0F;

// Controls
const float PAN_SPEED = 15.0F;
const float ROTATE_SPEED = 2.0F;
const float ZOOM_SPEED = 2.0F;
const float MIN_DISTANCE = 5.0F;
const float MAX_DISTANCE = 50.0F;

Camera::Camera()
    : target_({.x = 0.0F, .y = 0.0F, .z = 0.0F}), distance_(DEFAULT_DISTANCE), angle_(DEFAULT_ANGLE),
      pitch_(DEFAULT_PITCH) {}

void Camera::update() {
    float deltaTime = GetFrameTime();

    // Pan: WASD moves the target relative to current camera rotation.
    Vector3 forward = {.x = -sinf(angle_), .y = 0.0F, .z = -cosf(angle_)};
    Vector3 right = {.x = cosf(angle_), .y = 0.0F, .z = -sinf(angle_)};

    if (IsKeyDown(KEY_W)) {
        target_ = Vector3Add(target_, Vector3Scale(forward, PAN_SPEED * deltaTime));
    }
    if (IsKeyDown(KEY_S)) {
        target_ = Vector3Add(target_, Vector3Scale(forward, -PAN_SPEED * deltaTime));
    }
    if (IsKeyDown(KEY_D)) {
        target_ = Vector3Add(target_, Vector3Scale(right, PAN_SPEED * deltaTime));
    }
    if (IsKeyDown(KEY_A)) {
        target_ = Vector3Add(target_, Vector3Scale(right, -PAN_SPEED * deltaTime));
    }

    // Rotate: Q/E orbits around the target.
    if (IsKeyDown(KEY_Q)) {
        angle_ -= ROTATE_SPEED * deltaTime;
    }
    if (IsKeyDown(KEY_E)) {
        angle_ += ROTATE_SPEED * deltaTime;
    }

    // Zoom: scroll wheel.
    float wheel = GetMouseWheelMove();
    distance_ -= wheel * ZOOM_SPEED;
    distance_ = std::clamp(distance_, MIN_DISTANCE, MAX_DISTANCE);
}

Camera3D Camera::raw() const {
    // Compute camera position on a sphere around the target.
    Vector3 position = {
        .x = target_.x + (distance_ * cosf(pitch_) * sinf(angle_)),
        .y = target_.y + (distance_ * sinf(pitch_)),
        .z = target_.z + (distance_ * cosf(pitch_) * cosf(angle_)),
    };

    Camera3D cam = {};
    cam.position = position;
    cam.target = target_;
    cam.up = {.x = 0.0F, .y = 1.0F, .z = 0.0F};
    cam.fovy = DEFAULT_FOV;
    cam.projection = CAMERA_PERSPECTIVE;

    return cam;
}

} // namespace engine
