#include "engine/BuildingRenderer.h"

#include "engine/FactionColors.h"
#include "engine/HexGrid.h"
#include "engine/ModelManager.h"
#include "game/Building.h"

#include "raylib.h"

#include <algorithm>

namespace engine {

// ── Building model rendering constants ──────────────────────────────────────

/// Y-offset to place models at ground level.
static constexpr float MODEL_Y_OFFSET = 0.0F;

/// Uniform scale applied to building models so they fit within a hex tile.
static constexpr float MODEL_SCALE = 0.35F;

/// Scale factor for buildings that are under construction (half size).
static constexpr float CONSTRUCTION_SCALE_FACTOR = 0.5F;

// ── Fallback cylinder dimensions (used when no model is available) ──────────

static constexpr float FALLBACK_Y_OFFSET = 0.1F;
static constexpr float FALLBACK_RADIUS = 0.2F;
static constexpr float FALLBACK_HEIGHT = 0.15F;
static constexpr int FALLBACK_SLICES = 6;

/// Factor to dim building colors on explored-but-not-visible tiles.
static constexpr float BUILDING_DIM_FACTOR = 0.5F;

// ── Per-building-type colors (used when no faction is assigned) ──────────────

static constexpr Color CITY_CENTER_COLOR = {.r = 200, .g = 200, .b = 200, .a = 255};
static constexpr Color FARM_COLOR = GREEN;
static constexpr Color MINE_COLOR = BROWN;
static constexpr Color LUMBER_MILL_COLOR = DARKGREEN;
static constexpr Color BARRACKS_COLOR = MAROON;
static constexpr Color MARKET_COLOR = GOLD;
static constexpr Color DEFAULT_BUILDING_COLOR = GRAY;

static Color dimBuildingColor(Color base) {
    return Color{
        .r = static_cast<unsigned char>(static_cast<float>(base.r) * BUILDING_DIM_FACTOR),
        .g = static_cast<unsigned char>(static_cast<float>(base.g) * BUILDING_DIM_FACTOR),
        .b = static_cast<unsigned char>(static_cast<float>(base.b) * BUILDING_DIM_FACTOR),
        .a = base.a,
    };
}

/// Resolve a building-type color based on the building name.
static Color resolveBuildingTypeColor(const game::Building &building) {
    if (building.name() == "City Center") {
        return CITY_CENTER_COLOR;
    }
    if (building.name() == "Farm") {
        return FARM_COLOR;
    }
    if (building.name() == "Mine") {
        return MINE_COLOR;
    }
    if (building.name() == "Lumber Mill") {
        return LUMBER_MILL_COLOR;
    }
    if (building.name() == "Barracks") {
        return BARRACKS_COLOR;
    }
    if (building.name() == "Market") {
        return MARKET_COLOR;
    }
    return DEFAULT_BUILDING_COLOR;
}

/// Resolve the rendering color for a building, using faction color when available.
static Color resolveBuildingColor(const game::Building &building, const game::FactionRegistry &factions) {
    if (building.factionId() != 0) {
        const auto *faction = factions.findFaction(building.factionId());
        if (faction != nullptr) {
            return faction_colors::factionColor(faction->type(), faction->colorIndex());
        }
    }
    return resolveBuildingTypeColor(building);
}

/// Derive a lighter wire-frame color from the fill color.
static constexpr float WIRE_BRIGHTEN = 1.4F;
static constexpr float COLOR_CLAMP = 255.0F;

static Color wireColorFromFill(Color fill) {
    auto brighten = [](unsigned char channel) -> unsigned char {
        float val = std::min(static_cast<float>(channel) * WIRE_BRIGHTEN, COLOR_CLAMP);
        return static_cast<unsigned char>(val);
    };
    return Color{
        .r = brighten(fill.r),
        .g = brighten(fill.g),
        .b = brighten(fill.b),
        .a = fill.a,
    };
}

// ── Model registration ─────────────────────────────────────────────────────

void registerBuildingModels(ModelManager &models) {
    // City center: larger cube
    models.generateFallback("building_city_center", FallbackShape::Cube);
    // Farm: flat wide cube (fields)
    models.generateFallback("building_farm", FallbackShape::Cube);
    // Mine: cone shape
    models.generateFallback("building_mine", FallbackShape::Cone);
    // Lumber mill: cylinder
    models.generateFallback("building_lumber_mill", FallbackShape::Cylinder);
    // Barracks: tall cube
    models.generateFallback("building_barracks", FallbackShape::Cube);
    // Market: cylinder
    models.generateFallback("building_market", FallbackShape::Cylinder);
}

// ── Drawing ─────────────────────────────────────────────────────────────────

void drawBuildings(const std::vector<game::Building> &buildings, const ModelManager &models,
                   const game::FactionRegistry &factions, const game::FogOfWar *fog, game::FactionId playerFactionId) {
    for (const auto &building : buildings) {
        Color color = resolveBuildingColor(building, factions);

        for (const auto &tile : building.occupiedTiles()) {
            // Hide buildings on unexplored tiles.
            if (fog != nullptr) {
                auto vis = fog->getVisibility(playerFactionId, tile.row, tile.col);
                if (vis == game::VisibilityState::Unexplored) {
                    continue;
                }
                if (vis == game::VisibilityState::Explored) {
                    color = dimBuildingColor(color);
                }
            }

            Vector3 pos = hex::tileCenter(tile.row, tile.col);

            // Try to use a 3D model via ModelManager.
            const auto &key = building.modelKey();
            const Model *mdl = key.empty() ? nullptr : models.getModel(key);

            // Under-construction buildings are drawn at reduced scale.
            float scaleFactor = building.underConstruction() ? CONSTRUCTION_SCALE_FACTOR : 1.0F;

            if (mdl != nullptr) {
                pos.y = MODEL_Y_OFFSET;
                float scale = MODEL_SCALE * scaleFactor;
                Color wire = wireColorFromFill(color);
                DrawModel(*mdl, pos, scale, color);
                DrawModelWires(*mdl, pos, scale, wire);
            } else {
                // Fallback to the original cylinder rendering.
                pos.y = FALLBACK_Y_OFFSET;
                float radius = FALLBACK_RADIUS * scaleFactor;
                float height = FALLBACK_HEIGHT * scaleFactor;
                DrawCylinder(pos, radius, radius, height, FALLBACK_SLICES, color);
            }
        }
    }
}

} // namespace engine
