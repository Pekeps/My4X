#include "engine/CombatLogRenderer.h"

#include "game/CombatLog.h"

#include "raylib.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace engine {

namespace {

/// Background color for the combat log panel.
const Color LOG_BG = {16, 18, 26, 210};

/// Heading text color.
const Color LOG_HEADING_COL = {130, 140, 170, 255};

/// Normal combat event text color.
const Color LOG_TEXT_COL = {190, 190, 200, 255};

/// Damage number text color.
const Color LOG_DAMAGE_COL = {255, 100, 100, 255};

/// Counter-attack damage text color.
const Color LOG_COUNTER_COL = {255, 180, 80, 255};

/// Kill indicator text color.
const Color LOG_KILL_COL = {255, 60, 60, 255};

/// Dim text color for older events.
const Color LOG_DIM_COL = {120, 120, 140, 255};

/// Corner roundness for the combat log panel background.
constexpr float LOG_PANEL_ROUNDNESS = 0.04F;

/// Number of polygon segments for rounded corners.
constexpr int LOG_PANEL_SEGMENTS = 6;

/// Format a single combat event into a display string.
std::string formatCombatEvent(const game::CombatEvent &event) {
    std::string result;

    // Use stored names if available, otherwise fall back to faction IDs.
    std::string attackerLabel =
        event.attackerName.empty() ? ("Unit[F" + std::to_string(event.attackerFactionId) + "]") : event.attackerName;
    std::string defenderLabel =
        event.defenderName.empty() ? ("Unit[F" + std::to_string(event.defenderFactionId) + "]") : event.defenderName;

    // Append faction name in parentheses if available.
    if (!event.attackerFactionName.empty()) {
        attackerLabel += " (" + event.attackerFactionName + ")";
    }
    if (!event.defenderFactionName.empty()) {
        defenderLabel += " (" + event.defenderFactionName + ")";
    }

    result = attackerLabel + " -> " + defenderLabel + " " + std::to_string(event.damageToDefender) + " dmg";

    if (event.damageToAttacker > 0) {
        result += ", ctr: " + std::to_string(event.damageToAttacker);
    }

    if (event.defenderDied) {
        result += " [KILL]";
    }
    if (event.attackerDied) {
        result += " [DIED]";
    }

    return result;
}

} // namespace

void drawCombatLogPanel(const game::CombatLog &combatLog, int screenWidth, int screenHeight, int scrollOffset) {
    if (combatLog.empty()) {
        return;
    }

    // Get recent events for display (with scroll offset).
    auto totalCount = static_cast<int>(combatLog.size());
    int displayCount = std::min(totalCount, static_cast<int>(COMBAT_LOG_MAX_DISPLAY));

    // Clamp scroll offset to valid range.
    int maxScroll = std::max(0, totalCount - displayCount);
    int clampedScroll = std::clamp(scrollOffset, 0, maxScroll);

    // Fetch enough events to cover the scroll window.
    int fetchCount = displayCount + clampedScroll;
    auto events = combatLog.recentEvents(static_cast<std::size_t>(fetchCount));

    // Calculate panel dimensions.
    int panelH =
        (COMBAT_LOG_PAD * 2) + COMBAT_LOG_HEADING_SIZE + COMBAT_LOG_HEADING_GAP + (displayCount * COMBAT_LOG_LINE_H);
    int panelX = screenWidth - COMBAT_LOG_PANEL_W - COMBAT_LOG_MARGIN;
    int panelY = screenHeight - panelH - COMBAT_LOG_MARGIN;

    // Draw panel background.
    DrawRectangleRounded({static_cast<float>(panelX), static_cast<float>(panelY),
                          static_cast<float>(COMBAT_LOG_PANEL_W), static_cast<float>(panelH)},
                         LOG_PANEL_ROUNDNESS, LOG_PANEL_SEGMENTS, LOG_BG);

    int x = panelX + COMBAT_LOG_PAD;
    int y = panelY + COMBAT_LOG_PAD;

    // Draw heading.
    DrawText("COMBAT LOG", x, y, COMBAT_LOG_HEADING_SIZE, LOG_HEADING_COL);

    // Show scroll indicator if there are more events above.
    if (clampedScroll < maxScroll) {
        static constexpr int SCROLL_HINT_SIZE = 12;
        int hintX = panelX + COMBAT_LOG_PANEL_W - COMBAT_LOG_PAD - MeasureText("^ scroll", SCROLL_HINT_SIZE);
        DrawText("^ scroll", hintX, y, SCROLL_HINT_SIZE, LOG_DIM_COL);
    }

    y += COMBAT_LOG_HEADING_SIZE + COMBAT_LOG_HEADING_GAP;

    // Draw events (oldest first in the display window).
    // events is ordered oldest-to-newest. We want to show the subset [start..start+displayCount).
    int startIdx = static_cast<int>(events.size()) - displayCount;
    for (int i = 0; i < displayCount; ++i) {
        int eventIdx = startIdx + i;
        if (eventIdx < 0 || std::cmp_greater_equal(eventIdx, events.size())) {
            continue;
        }

        const auto &event = events[static_cast<std::size_t>(eventIdx)];
        std::string text = formatCombatEvent(event);

        // Older events are dimmer.
        Color textColor = (i < displayCount / 2) ? LOG_DIM_COL : LOG_TEXT_COL;

        // Highlight kills.
        if (event.defenderDied || event.attackerDied) {
            textColor = LOG_KILL_COL;
        }

        DrawText(text.c_str(), x, y, COMBAT_LOG_FONT_SIZE, textColor);
        y += COMBAT_LOG_LINE_H;
    }
}

} // namespace engine
