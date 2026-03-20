#pragma once

#include "game/DiplomacyManager.h"

#include "raylib.h"

#include <cstdint>

namespace engine::diplomacy_colors {

// ── Diplomacy indicator colors ──────────────────────────────────────────────

/// Red border for factions at war with the player.
static constexpr Color WAR_COLOR = {.r = 220, .g = 40, .b = 40, .a = 255};

/// Green border for allied factions.
static constexpr Color ALLIANCE_COLOR = {.r = 40, .g = 200, .b = 80, .a = 255};

/// Gray border for factions at peace (neutral diplomatic stance).
static constexpr Color PEACE_COLOR = {.r = 140, .g = 140, .b = 150, .a = 255};

/// Diplomacy ring rendering constants.
static constexpr float DIPLOMACY_RING_RADIUS = 0.45F;
static constexpr float DIPLOMACY_RING_HEIGHT = 0.03F;
static constexpr int DIPLOMACY_RING_SLICES = 16;

/// Return the color representing the given diplomatic state.
inline Color diplomacyColor(game::DiplomacyState state) {
    switch (state) {
    case game::DiplomacyState::War:
        return WAR_COLOR;
    case game::DiplomacyState::Alliance:
        return ALLIANCE_COLOR;
    case game::DiplomacyState::Peace:
        return PEACE_COLOR;
    }
    return PEACE_COLOR;
}

/// Return a short label for the diplomatic state.
inline const char *diplomacyLabel(game::DiplomacyState state) {
    switch (state) {
    case game::DiplomacyState::War:
        return "War";
    case game::DiplomacyState::Alliance:
        return "Allied";
    case game::DiplomacyState::Peace:
        return "Peace";
    }
    return "Peace";
}

} // namespace engine::diplomacy_colors
