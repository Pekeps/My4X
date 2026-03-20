#pragma once

#include "game/DiplomacyManager.h"
#include "game/Faction.h"
#include "game/FactionRegistry.h"
#include "game/FogOfWar.h"
#include "game/GameState.h"
#include "game/Map.h"
#include "game/Unit.h"

#include "raylib.h"

#include <memory>
#include <string>
#include <vector>

namespace engine {

/// Draws a hover tooltip at the mouse cursor showing information about the
/// tile, terrain, and any unit or city located on the hovered hex.
///
/// The tooltip appears after a short hover delay to avoid flicker.
/// Call update() each frame before draw().
class Tooltip {
  public:
    /// Update hover state: track which tile the mouse is over and how long.
    /// @param hoveredTile  The tile coordinate under the mouse, or nullopt.
    /// @param dt           Frame delta time in seconds.
    void update(int hoveredRow, int hoveredCol, bool hasHover, float dt);

    /// Draw the tooltip near the mouse cursor. Call in 2D mode.
    /// @param state         The game state to query for info.
    /// @param playerFaction The player faction ID (for fog of war).
    void draw(const game::GameState &state, game::FactionId playerFaction) const;

  private:
    /// Build tooltip text lines for the hovered tile.
    [[nodiscard]] std::vector<std::string> buildLines(const game::GameState &state,
                                                      game::FactionId playerFaction) const;

    // ── Layout constants ────────────────────────────────────────────────────

    /// Seconds of hovering before the tooltip appears.
    static constexpr float HOVER_DELAY = 0.35F;

    /// Font size for tooltip text.
    static constexpr int FONT_SIZE = 14;

    /// Line height in pixels.
    static constexpr int LINE_HEIGHT = 18;

    /// Padding inside the tooltip box.
    static constexpr int PADDING = 8;

    /// Horizontal offset from the mouse cursor.
    static constexpr int CURSOR_OFFSET_X = 16;

    /// Vertical offset from the mouse cursor.
    static constexpr int CURSOR_OFFSET_Y = 16;

    /// Tooltip background color.
    static constexpr Color BG_COLOR = {16, 18, 26, 220};

    /// Tooltip border color.
    static constexpr Color BORDER_COLOR = {80, 90, 120, 255};

    /// Tooltip text color.
    static constexpr Color TEXT_COLOR = {210, 210, 220, 255};

    /// Tooltip accent (header) color.
    static constexpr Color ACCENT_COLOR = {220, 200, 120, 255};

    /// Screen edge margin to prevent clipping.
    static constexpr int SCREEN_MARGIN = 8;

    // ── State ───────────────────────────────────────────────────────────────

    int hoverRow_ = -1;
    int hoverCol_ = -1;
    float hoverTime_ = 0.0F;
    bool hovering_ = false;
};

} // namespace engine
