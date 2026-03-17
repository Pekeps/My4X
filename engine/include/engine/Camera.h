#pragma once

#include "raylib.h"

namespace engine {

// Orbital camera that looks down at the map.
// Pans in X/Z, rotates around the Y axis (vertical in raylib's 3D).
// The map lies on the XZ plane.
class Camera {
  public:
    Camera();

    // Call each frame to handle WASD pan, Q/E rotate, scroll zoom.
    void update();

    // Use between BeginMode3D / EndMode3D.
    [[nodiscard]] Camera3D raw() const;

  private:
    Vector3 target_;   // Point the camera looks at (on the XZ plane).
    float distance_;   // Distance from target.
    float angle_;      // Rotation around the Y axis in radians.
    float pitch_;      // Tilt angle from horizontal in radians.
};

} // namespace engine
