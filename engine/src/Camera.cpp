#include "engine/Camera.h"

#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>

namespace engine {

const float DEFAULT_DISTANCE = 30.0F;
const float DEFAULT_ANGLE = 0.0F;
const float DEFAULT_FOV = 45.0F;

const float PAN_SPEED = 15.0F;
const float ROTATE_SPEED = 2.0F;
const float ZOOM_SPEED = 2.0F;
const float MIN_DISTANCE = 5.0F;
const float MAX_DISTANCE = 60.0F;

// Pitch range: lerped based on zoom distance.
// At MIN_DISTANCE (fully zoomed in): max tilt (looking at an angle).
// At MAX_DISTANCE (fully zoomed out): top-down view.
const float MIN_PITCH_AT_DISTANCE = 20.0F; // ~34 degrees from horizontal (max tilt when zoomed in)
const float MIN_PITCH = 0.6F;              // ~34 degrees from horizontal (max tilt when zoomed in)
const float MAX_PITCH = 1.55F;             // ~89 degrees from horizontal (nearly top-down when zoomed out)

const float INIT_CAMERA_X = 10.0F;
const float INIT_CAMERA_Y = 0.0F;
const float INIT_CAMERA_Z = 10.0F;

Camera::Camera()
    : target_({.x = INIT_CAMERA_X, .y = INIT_CAMERA_Y, .z = INIT_CAMERA_Z}), distance_(DEFAULT_DISTANCE),
      angle_(DEFAULT_ANGLE) {}

void Camera::update() {
    float deltaTime = GetFrameTime();

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

    if (IsKeyDown(KEY_Q)) {
        angle_ -= ROTATE_SPEED * deltaTime;
    }
    if (IsKeyDown(KEY_E)) {
        angle_ += ROTATE_SPEED * deltaTime;
    }

    float wheel = GetMouseWheelMove();
    distance_ -= wheel * ZOOM_SPEED;
    distance_ = std::clamp(distance_, MIN_DISTANCE, MAX_DISTANCE);
}

Camera3D Camera::raw() const {
    // Pitch lerps from max tilt to top-down only when zooming OUT past default distance.
    // Zooming in closer than default keeps max tilt.
    float zoomFactor = (distance_ - MIN_PITCH_AT_DISTANCE) / (MAX_DISTANCE - MIN_PITCH_AT_DISTANCE);
    zoomFactor = std::clamp(zoomFactor, 0.0F, 1.0F);
    float pitch = Lerp(MIN_PITCH, MAX_PITCH, zoomFactor);

    Vector3 position = {
        .x = target_.x + (distance_ * cosf(pitch) * sinf(angle_)),
        .y = target_.y + (distance_ * sinf(pitch)),
        .z = target_.z + (distance_ * cosf(pitch) * cosf(angle_)),
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
