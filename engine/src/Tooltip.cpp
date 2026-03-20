#include "engine/Tooltip.h"

#include "game/TerrainType.h"

#include <algorithm>
#include <string>

namespace engine {

void Tooltip::update(int hoveredRow, int hoveredCol, bool hasHover, float dt) {
    if (!hasHover) {
        hovering_ = false;
        hoverTime_ = 0.0F;
        return;
    }
    if (hoveredRow != hoverRow_ || hoveredCol != hoverCol_) {
        // Mouse moved to a different tile — reset timer.
        hoverRow_ = hoveredRow;
        hoverCol_ = hoveredCol;
        hoverTime_ = 0.0F;
    }
    hovering_ = true;
    hoverTime_ += dt;
}

std::vector<std::string> Tooltip::buildLines(const game::GameState &state, game::FactionId playerFaction) const {
    std::vector<std::string> lines;

    // Check fog of war — don't show info for unexplored tiles.
    auto vis = state.fogOfWar().getVisibility(playerFaction, hoverRow_, hoverCol_);
    if (vis == game::VisibilityState::Unexplored) {
        lines.emplace_back("Unexplored");
        return lines;
    }

    // Tile coordinates.
    lines.emplace_back("Tile (" + std::to_string(hoverRow_) + ", " + std::to_string(hoverCol_) + ")");

    // Terrain info.
    const auto &tile = state.map().tile(hoverRow_, hoverCol_);
    auto terrain = tile.terrainType();
    const auto &props = game::getTerrainProperties(terrain);

    std::string terrainLine = "Terrain: " + std::string(game::terrainName(terrain));
    lines.push_back(terrainLine);

    if (props.movementCost > 0) {
        lines.emplace_back("Move cost: " + std::to_string(props.movementCost));
    } else {
        lines.emplace_back("Impassable");
    }

    if (props.defenseModifier > 0) {
        lines.emplace_back("Defense: +" + std::to_string(props.defenseModifier));
    } else if (props.defenseModifier < 0) {
        lines.emplace_back("Defense: " + std::to_string(props.defenseModifier));
    }

    // Check for a unit on this tile (only if currently visible).
    if (vis == game::VisibilityState::Visible) {
        for (const auto &unit : state.units()) {
            if (unit->isAlive() && unit->row() == hoverRow_ && unit->col() == hoverCol_) {
                lines.emplace_back("---");
                lines.emplace_back(unit->name());
                lines.emplace_back("HP: " + std::to_string(unit->health()) + "/" + std::to_string(unit->maxHealth()));
                lines.emplace_back("ATK: " + std::to_string(unit->attack()) +
                                   "  DEF: " + std::to_string(unit->defense()));
                lines.emplace_back("Moves: " + std::to_string(unit->movementRemaining()) + "/" +
                                   std::to_string(unit->movement()));
                if (unit->attackRange() > 1) {
                    lines.emplace_back("Range: " + std::to_string(unit->attackRange()));
                }

                // Show faction name.
                const auto *faction = state.factionRegistry().findFaction(unit->factionId());
                if (faction != nullptr) {
                    lines.emplace_back("Faction: " + faction->name());
                }
                break; // Only show the first unit on the tile.
            }
        }

        // Check for a city on this tile.
        for (const auto &city : state.cities()) {
            if (city.containsTile(hoverRow_, hoverCol_)) {
                lines.emplace_back("---");
                lines.emplace_back("City: " + city.name());
                lines.emplace_back("Pop: " + std::to_string(city.population()));
                break;
            }
        }
    }

    return lines;
}

void Tooltip::draw(const game::GameState &state, game::FactionId playerFaction) const {
    if (!hovering_ || hoverTime_ < HOVER_DELAY) {
        return;
    }

    auto lines = buildLines(state, playerFaction);
    if (lines.empty()) {
        return;
    }

    // Measure tooltip dimensions.
    int maxWidth = 0;
    for (const auto &line : lines) {
        int lineWidth = MeasureText(line.c_str(), FONT_SIZE);
        maxWidth = std::max(maxWidth, lineWidth);
    }

    int tooltipW = maxWidth + (PADDING * 2);
    int tooltipH = (static_cast<int>(lines.size()) * LINE_HEIGHT) + (PADDING * 2);

    // Position near cursor, clamped to screen.
    int mouseX = GetMouseX();
    int mouseY = GetMouseY();
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    int posX = mouseX + CURSOR_OFFSET_X;
    int posY = mouseY + CURSOR_OFFSET_Y;

    // Clamp to screen edges.
    if (posX + tooltipW > screenW - SCREEN_MARGIN) {
        posX = mouseX - tooltipW - SCREEN_MARGIN;
    }
    if (posY + tooltipH > screenH - SCREEN_MARGIN) {
        posY = mouseY - tooltipH - SCREEN_MARGIN;
    }
    posX = std::max(posX, SCREEN_MARGIN);
    posY = std::max(posY, SCREEN_MARGIN);

    // Draw background and border.
    DrawRectangle(posX, posY, tooltipW, tooltipH, BG_COLOR);
    DrawRectangleLines(posX, posY, tooltipW, tooltipH, BORDER_COLOR);

    // Draw text lines.
    int textX = posX + PADDING;
    int textY = posY + PADDING;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        Color col = (i == 0) ? ACCENT_COLOR : TEXT_COLOR;
        if (lines[i] == "---") {
            // Draw a separator line instead of text.
            int sepY = textY + (LINE_HEIGHT / 2);
            DrawLine(textX, sepY, textX + maxWidth, sepY, BORDER_COLOR);
        } else {
            DrawText(lines[i].c_str(), textX, textY, FONT_SIZE, col);
        }
        textY += LINE_HEIGHT;
    }
}

} // namespace engine
