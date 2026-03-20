#pragma once

#include "raylib.h"

#include <cstdint>
#include <string>
#include <vector>

namespace engine {

/// The possible screens of the game.
enum class Screen : std::uint8_t { MainMenu, Settings, InGame };

/// Result from a menu button click.
enum class MenuAction : std::uint8_t { None, NewGame, LoadGame, Settings, Quit, BackToMenu, ApplySettings };

/// A clickable menu button with hover/press visual feedback.
struct MenuButton {
    std::string label;
    MenuAction action;
    Rectangle bounds;
    bool hovered = false;
    bool pressed = false;
};

/// Renders and manages the main menu screen (title + buttons).
class MainMenu {
  public:
    MainMenu();

    /// Process input and return any action triggered this frame.
    [[nodiscard]] MenuAction update(int screenWidth, int screenHeight);

    /// Draw the main menu. Call in 2D mode.
    void draw(int screenWidth, int screenHeight) const;

  private:
    /// Recompute button layout based on current screen dimensions.
    void layoutButtons(int screenWidth, int screenHeight);

    // ── Layout constants ────────────────────────────────────────────────────

    /// Title font size.
    static constexpr int TITLE_FONT_SIZE = 64;

    /// Subtitle font size.
    static constexpr int SUBTITLE_FONT_SIZE = 20;

    /// Button font size.
    static constexpr int BUTTON_FONT_SIZE = 24;

    /// Button width.
    static constexpr int BUTTON_WIDTH = 280;

    /// Button height.
    static constexpr int BUTTON_HEIGHT = 50;

    /// Vertical spacing between buttons.
    static constexpr int BUTTON_SPACING = 16;

    /// Vertical offset of the title from center.
    static constexpr int TITLE_Y_OFFSET = 200;

    /// Vertical offset of subtitle below title.
    static constexpr int SUBTITLE_Y_OFFSET = 80;

    /// Corner rounding for buttons.
    static constexpr float BUTTON_ROUNDNESS = 0.2F;

    /// Number of segments for rounded rectangles.
    static constexpr int BUTTON_SEGMENTS = 6;

    // ── Colors ──────────────────────────────────────────────────────────────

    static constexpr Color BG_COLOR = {12, 14, 22, 255};
    static constexpr Color TITLE_COLOR = {220, 200, 120, 255};
    static constexpr Color SUBTITLE_COLOR = {140, 140, 160, 255};
    static constexpr Color BUTTON_BG = {30, 34, 50, 255};
    static constexpr Color BUTTON_HOVER_BG = {50, 58, 80, 255};
    static constexpr Color BUTTON_PRESS_BG = {70, 78, 100, 255};
    static constexpr Color BUTTON_BORDER = {80, 90, 120, 255};
    static constexpr Color BUTTON_TEXT = {210, 210, 220, 255};
    static constexpr Color BUTTON_TEXT_DIM = {120, 120, 140, 255};
    static constexpr Color VERSION_COLOR = {80, 80, 100, 255};

    /// Version text font size.
    static constexpr int VERSION_FONT_SIZE = 14;

    /// Version text margin from bottom.
    static constexpr int VERSION_MARGIN = 12;

    std::vector<MenuButton> buttons_;
};

/// Renders and manages a basic settings screen.
class SettingsScreen {
  public:
    SettingsScreen();

    /// Process input and return any action triggered.
    [[nodiscard]] MenuAction update(int screenWidth, int screenHeight);

    /// Draw the settings screen. Call in 2D mode.
    void draw(int screenWidth, int screenHeight) const;

  private:
    void layoutButtons(int screenWidth, int screenHeight);

    // ── Layout constants ────────────────────────────────────────────────────

    /// Heading font size.
    static constexpr int HEADING_FONT_SIZE = 40;

    /// Label font size.
    static constexpr int LABEL_FONT_SIZE = 20;

    /// Value font size.
    static constexpr int VALUE_FONT_SIZE = 20;

    /// Vertical spacing between setting rows.
    static constexpr int ROW_SPACING = 50;

    /// Heading Y offset from top.
    static constexpr int HEADING_Y_OFFSET = 80;

    /// Settings area Y start offset from top.
    static constexpr int SETTINGS_Y_START = 180;

    /// Label X offset from center.
    static constexpr int LABEL_X_OFFSET = 200;

    /// Value X offset from center.
    static constexpr int VALUE_X_OFFSET = 40;

    /// Back button width.
    static constexpr int BACK_BUTTON_W = 180;

    /// Back button height.
    static constexpr int BACK_BUTTON_H = 44;

    /// Back button margin from bottom.
    static constexpr int BACK_BUTTON_MARGIN = 60;

    /// Button rounding.
    static constexpr float BACK_BUTTON_ROUNDNESS = 0.2F;

    /// Button segments.
    static constexpr int BACK_BUTTON_SEGMENTS = 6;

    // ── Colors ──────────────────────────────────────────────────────────────

    static constexpr Color BG_COLOR = {12, 14, 22, 255};
    static constexpr Color HEADING_COLOR = {220, 200, 120, 255};
    static constexpr Color LABEL_COLOR = {180, 180, 195, 255};
    static constexpr Color VALUE_COLOR = {140, 200, 255, 255};
    static constexpr Color DIM_COLOR = {100, 100, 120, 255};
    static constexpr Color BUTTON_BG = {30, 34, 50, 255};
    static constexpr Color BUTTON_HOVER_BG = {50, 58, 80, 255};
    static constexpr Color BUTTON_PRESS_BG = {70, 78, 100, 255};
    static constexpr Color BUTTON_BORDER = {80, 90, 120, 255};
    static constexpr Color BUTTON_TEXT = {210, 210, 220, 255};

    MenuButton backButton_;
};

} // namespace engine
