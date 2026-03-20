#pragma once

#include "raylib.h"

#include <cstdint>

namespace engine {

/// Fade transition directions.
enum class TransitionDir : std::uint8_t { FadeIn, FadeOut };

/// Manages a simple full-screen fade-to-black transition effect.
///
/// Usage:
///   1. Call start(FadeOut) to begin fading the screen to black.
///   2. Call update(dt) each frame.
///   3. When isComplete() becomes true after a FadeOut, switch screens and
///      call start(FadeIn) to fade back in.
///   4. Draw the overlay with draw() each frame (after all other 2D drawing).
class Transition {
  public:
    /// Start a new transition in the given direction.
    void start(TransitionDir dir);

    /// Advance the transition by delta-time seconds.
    void update(float dt);

    /// Draw the full-screen semi-transparent overlay. Call in 2D mode.
    void draw(int screenWidth, int screenHeight) const;

    /// Returns true if a transition is currently running.
    [[nodiscard]] bool isActive() const;

    /// Returns true once the current transition has finished.
    [[nodiscard]] bool isComplete() const;

    /// Returns the current alpha (0.0 = fully transparent, 1.0 = fully black).
    [[nodiscard]] float alpha() const;

  private:
    /// Duration of a single fade in seconds.
    static constexpr float FADE_DURATION = 0.35F;

    /// Maximum alpha byte value.
    static constexpr int MAX_ALPHA = 255;

    TransitionDir direction_ = TransitionDir::FadeIn;
    float progress_ = 0.0F;
    bool active_ = false;
    bool complete_ = false;
};

} // namespace engine
