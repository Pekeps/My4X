#pragma once

#include "game/Faction.h"

#include "raylib.h"

#include <array>
#include <cstdint>

namespace engine::faction_colors {

// ── Player palette (indices 0..7) ────────────────────────────────────────────

static constexpr std::size_t PLAYER_PALETTE_SIZE = 8;

/// The 8 player faction colors, ordered by color index.
static constexpr std::array<Color, PLAYER_PALETTE_SIZE> PLAYER_PALETTE = {{
    {.r = 200, .g = 40, .b = 40, .a = 255},  // 0 — Red
    {.r = 40, .g = 80, .b = 200, .a = 255},  // 1 — Blue
    {.r = 40, .g = 180, .b = 60, .a = 255},  // 2 — Green
    {.r = 200, .g = 160, .b = 40, .a = 255}, // 3 — Gold
    {.r = 160, .g = 50, .b = 200, .a = 255}, // 4 — Purple
    {.r = 40, .g = 180, .b = 200, .a = 255}, // 5 — Cyan
    {.r = 220, .g = 120, .b = 40, .a = 255}, // 6 — Orange
    {.r = 200, .g = 80, .b = 160, .a = 255}, // 7 — Pink
}};

// ── Wire (outline) variants — slightly brighter/lighter ─────────────────────

static constexpr std::array<Color, PLAYER_PALETTE_SIZE> PLAYER_PALETTE_WIRE = {{
    {.r = 255, .g = 80, .b = 80, .a = 255},   // 0 — Red wire
    {.r = 80, .g = 120, .b = 255, .a = 255},  // 1 — Blue wire
    {.r = 80, .g = 220, .b = 100, .a = 255},  // 2 — Green wire
    {.r = 255, .g = 210, .b = 80, .a = 255},  // 3 — Gold wire
    {.r = 200, .g = 100, .b = 255, .a = 255}, // 4 — Purple wire
    {.r = 80, .g = 220, .b = 255, .a = 255},  // 5 — Cyan wire
    {.r = 255, .g = 170, .b = 80, .a = 255},  // 6 — Orange wire
    {.r = 255, .g = 130, .b = 200, .a = 255}, // 7 — Pink wire
}};

// ── Neutral faction colors ──────────────────────────────────────────────────

static constexpr Color NEUTRAL_PASSIVE_COLOR = {.r = 140, .g = 140, .b = 140, .a = 255};
static constexpr Color NEUTRAL_PASSIVE_WIRE = {.r = 180, .g = 180, .b = 180, .a = 255};
static constexpr Color NEUTRAL_HOSTILE_COLOR = {.r = 140, .g = 30, .b = 30, .a = 255};
static constexpr Color NEUTRAL_HOSTILE_WIRE = {.r = 200, .g = 60, .b = 60, .a = 255};

// ── City territory tint variants (semi-transparent) ─────────────────────────

static constexpr std::uint8_t CITY_FILL_ALPHA = 100;
static constexpr std::uint8_t CITY_FILL_SELECTED_ALPHA = 160;

/// Maps faction color indices to raylib Color values.
///
/// Provides at least 8 visually distinct player colors plus dedicated
/// neutral-faction schemes (gray tones for passive, dark red for hostile).
/// Selection and hover highlights (e.g., YELLOW) are intentionally excluded
/// from this palette so they remain clearly distinguishable.

/// Return the fill color for a faction, given its type and color index.
/// Player factions use the palette; neutrals use their dedicated scheme.
inline Color factionColor(game::FactionType type, std::uint8_t colorIndex) {
    if (type == game::FactionType::NeutralPassive) {
        return NEUTRAL_PASSIVE_COLOR;
    }
    if (type == game::FactionType::NeutralHostile) {
        return NEUTRAL_HOSTILE_COLOR;
    }
    return PLAYER_PALETTE.at(colorIndex % PLAYER_PALETTE_SIZE);
}

/// Return the wire/outline color for a faction.
inline Color factionWireColor(game::FactionType type, std::uint8_t colorIndex) {
    if (type == game::FactionType::NeutralPassive) {
        return NEUTRAL_PASSIVE_WIRE;
    }
    if (type == game::FactionType::NeutralHostile) {
        return NEUTRAL_HOSTILE_WIRE;
    }
    return PLAYER_PALETTE_WIRE.at(colorIndex % PLAYER_PALETTE_SIZE);
}

/// Return a semi-transparent city territory tint for a faction.
inline Color cityTerritoryColor(game::FactionType type, std::uint8_t colorIndex, bool selected) {
    Color base = factionColor(type, colorIndex);
    base.a = selected ? CITY_FILL_SELECTED_ALPHA : CITY_FILL_ALPHA;
    return base;
}

} // namespace engine::faction_colors
