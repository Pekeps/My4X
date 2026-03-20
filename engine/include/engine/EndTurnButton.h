#pragma once

#include "raylib.h"

namespace engine {

/// A prominent "End Turn" button drawn at the bottom-center of the screen.
///
/// Returns true from update() when the player clicks it.
/// Also shows the current turn number.
class EndTurnButton {
  public:
    /// Process input. Returns true if the button was clicked.
    [[nodiscard]] bool update(int screenWidth, int screenHeight);

    /// Draw the button. Call in 2D mode.
    void draw(int screenWidth, int screenHeight, int currentTurn) const;

    // ── Layout constants (public for free-function helpers) ──────────────

    /// Button width in pixels.
    static constexpr int WIDTH = 160;

    /// Button height in pixels.
    static constexpr int HEIGHT = 44;

    /// Y offset from bottom of screen.
    static constexpr int BOTTOM_MARGIN = 24;

  private:
    // ── Layout constants ────────────────────────────────────────────────────

    /// Font size for button label.
    static constexpr int FONT_SIZE = 22;

    /// Font size for turn counter label above the button.
    static constexpr int TURN_FONT_SIZE = 16;

    /// Gap between turn label and button top.
    static constexpr int TURN_LABEL_GAP = 6;

    /// Button rounding.
    static constexpr float ROUNDNESS = 0.25F;

    /// Button segments.
    static constexpr int SEGMENTS = 6;

    // ── Colors ──────────────────────────────────────────────────────────────

    static constexpr Color BG_COLOR = {40, 100, 60, 255};
    static constexpr Color HOVER_BG = {60, 140, 80, 255};
    static constexpr Color PRESS_BG = {80, 170, 100, 255};
    static constexpr Color BORDER_COLOR = {100, 200, 130, 255};
    static constexpr Color TEXT_COLOR = {240, 240, 240, 255};
    static constexpr Color TURN_TEXT_COLOR = {180, 180, 200, 255};

    bool hovered_ = false;
    bool pressed_ = false;
};

} // namespace engine
