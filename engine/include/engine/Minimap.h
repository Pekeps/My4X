#pragma once

#include "game/City.h"
#include "game/Faction.h"
#include "game/FactionRegistry.h"
#include "game/FogOfWar.h"
#include "game/Map.h"
#include "game/Unit.h"

#include "raylib.h"

#include <memory>
#include <vector>

/// Renders a small 2D minimap overlay in the bottom-right corner of the screen.
///
/// Shows terrain colors, fog of war, unit/city markers in faction colors,
/// and a white rectangle indicating the camera viewport.
///
/// This is a pure 2D overlay drawn after EndMode3D().
namespace engine::minimap {

// ── Minimap layout constants ────────────────────────────────────────────────

/// Width of the minimap in screen pixels.
static constexpr int WIDTH = 200;

/// Height of the minimap in screen pixels.
static constexpr int HEIGHT = 150;

/// Margin from screen edges in pixels.
static constexpr int MARGIN = 12;

/// Padding inside the minimap border.
static constexpr int PADDING = 2;

/// Border thickness in pixels.
static constexpr int BORDER_THICKNESS = 1;

/// Size of unit dots on the minimap in pixels.
static constexpr int UNIT_DOT_SIZE = 3;

/// Size of city markers on the minimap in pixels.
static constexpr int CITY_MARKER_SIZE = 5;

/// Alpha value for dimming explored-but-not-visible tiles.
static constexpr unsigned char FOG_DIM_ALPHA = 120;

/// Alpha value for the viewport indicator rectangle lines.
static constexpr unsigned char VIEWPORT_ALPHA = 200;

/// Viewport indicator line thickness in pixels.
static constexpr int VIEWPORT_LINE_THICKNESS = 1;

// ── Colors ──────────────────────────────────────────────────────────────────

/// Background color of the minimap panel.
static constexpr Color BACKGROUND_COLOR = {.r = 16, .g = 18, .b = 26, .a = 230};

/// Border color of the minimap panel.
static constexpr Color BORDER_COLOR = {.r = 60, .g = 65, .b = 90, .a = 255};

/// Color for unexplored (fog of war) tiles.
static constexpr Color UNEXPLORED_COLOR = {.r = 0, .g = 0, .b = 0, .a = 255};

/// Color for the camera viewport indicator rectangle.
static constexpr Color VIEWPORT_COLOR = {.r = 255, .g = 255, .b = 255, .a = VIEWPORT_ALPHA};

/// Draw the minimap overlay.
///
/// Should be called in 2D mode (after EndMode3D, before EndDrawing).
/// @param map          The game map (terrain data).
/// @param fog          Fog of war state (nullable; if null, everything is visible).
/// @param playerFactionId The faction whose fog of war to apply.
/// @param units        All units in the game.
/// @param cities       All cities in the game.
/// @param factionRegistry Registry for looking up faction colors.
/// @param camera       The current 3D camera (used to compute viewport indicator).
/// @param screenWidth  Current screen width in pixels.
/// @param screenHeight Current screen height in pixels.
void draw(const game::Map &map, const game::FogOfWar *fog, game::FactionId playerFactionId,
          const std::vector<std::unique_ptr<game::Unit>> &units, const std::vector<game::City> &cities,
          const game::FactionRegistry &factionRegistry, Camera3D camera, int screenWidth, int screenHeight);

} // namespace engine::minimap
