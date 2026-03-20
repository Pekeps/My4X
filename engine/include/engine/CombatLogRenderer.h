#pragma once

#include "game/CombatLog.h"

#include "raylib.h"

namespace engine {

// ── Combat log HUD constants ─────────────────────────────────────────────────

/// Maximum number of recent combat events to display in the HUD.
static constexpr int COMBAT_LOG_MAX_DISPLAY = 8;

/// Width of the combat log panel in pixels.
static constexpr int COMBAT_LOG_PANEL_W = 480;

/// Height of each combat log entry line in pixels.
static constexpr int COMBAT_LOG_LINE_H = 18;

/// Font size for combat log entry text.
static constexpr int COMBAT_LOG_FONT_SIZE = 13;

/// Font size for the combat log heading.
static constexpr int COMBAT_LOG_HEADING_SIZE = 14;

/// Padding inside the combat log panel.
static constexpr int COMBAT_LOG_PAD = 10;

/// Margin from the bottom-right corner of the screen.
static constexpr int COMBAT_LOG_MARGIN = 14;

/// Vertical gap between the heading and the first entry.
static constexpr int COMBAT_LOG_HEADING_GAP = 6;

/// Draws the combat log panel in the bottom-right corner of the screen.
///
/// Shows the most recent combat events with attacker, defender, damage dealt,
/// and counter-attack damage. Scrollable view of the last N events.
///
/// @param combatLog The game-layer combat log to read events from.
/// @param screenWidth Width of the screen in pixels (for positioning).
/// @param screenHeight Height of the screen in pixels (for positioning).
/// @param scrollOffset Number of entries to scroll back (0 = most recent).
void drawCombatLogPanel(const game::CombatLog &combatLog, int screenWidth, int screenHeight, int scrollOffset = 0);

} // namespace engine
