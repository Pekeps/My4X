#include "engine/Minimap.h"

#include "engine/FactionColors.h"
#include "engine/HexGrid.h"
#include "engine/TerrainColors.h"
#include "game/FogOfWar.h"

#include "raylib.h"

#include <algorithm>
#include <cmath>

namespace engine::minimap {

// ── Internal constants ──────────────────────────────────────────────────────

/// Minimum cell size in pixels (ensures tiny maps don't get invisible cells).
static constexpr int MIN_CELL_SIZE = 1;

/// Half divisor used when centering the map within the minimap area.
static constexpr float HALF = 0.5F;

/// Alpha blend factor for dimming explored tiles (applied multiplicatively).
static constexpr float DIM_FACTOR = 0.5F;

/// Small epsilon for floating-point near-zero comparisons.
static constexpr float RAY_EPSILON = 1e-6F;

/// Odd row index used to compute max world X (odd rows have the rightward hex offset).
static constexpr int ODD_ROW_INDEX = 1;

/// Multiplier for the inset of border+padding on each side.
static constexpr int INSET_SIDES = 2;

// ── Helper: apply fog dimming to a color ────────────────────────────────────

static Color dimColor(Color base) {
    return {
        .r = static_cast<unsigned char>(static_cast<float>(base.r) * DIM_FACTOR),
        .g = static_cast<unsigned char>(static_cast<float>(base.g) * DIM_FACTOR),
        .b = static_cast<unsigned char>(static_cast<float>(base.b) * DIM_FACTOR),
        .a = base.a,
    };
}

// ── Helper: compute minimap cell layout ─────────────────────────────────────

struct MinimapLayout {
    /// Top-left corner of the minimap content area (inside border/padding).
    int contentX;
    int contentY;
    /// Available width/height for the map content.
    int contentW;
    int contentH;
    /// Size of each cell (pixels per tile).
    int cellW;
    int cellH;
    /// Offset to center the map within content area.
    int offsetX;
    int offsetY;
};

static MinimapLayout computeLayout(int screenWidth, int screenHeight, int mapRows, int mapCols) {
    MinimapLayout layout{};

    // Panel position (bottom-right).
    int panelX = screenWidth - WIDTH - MARGIN;
    int panelY = screenHeight - HEIGHT - MARGIN;
    int inset = BORDER_THICKNESS + PADDING;

    // Content area (inside border and padding).
    layout.contentX = panelX + inset;
    layout.contentY = panelY + inset;
    layout.contentW = WIDTH - (inset * INSET_SIDES);
    layout.contentH = HEIGHT - (inset * INSET_SIDES);

    // Compute cell size to fit the entire map.
    layout.cellW = std::max(MIN_CELL_SIZE, layout.contentW / mapCols);
    layout.cellH = std::max(MIN_CELL_SIZE, layout.contentH / mapRows);

    // Use uniform cell size (the smaller of the two) to preserve aspect ratio.
    int cellSize = std::min(layout.cellW, layout.cellH);
    layout.cellW = cellSize;
    layout.cellH = cellSize;

    // Center the map within the content area.
    int totalMapW = mapCols * layout.cellW;
    int totalMapH = mapRows * layout.cellH;
    layout.offsetX = (layout.contentW - totalMapW) / 2;
    layout.offsetY = (layout.contentH - totalMapH) / 2;

    return layout;
}

// ── Helper: convert tile (row, col) to minimap pixel position ───────────────

static void tileToMinimap(const MinimapLayout &layout, int row, int col, int &outX, int &outY) {
    outX = layout.contentX + layout.offsetX + (col * layout.cellW);
    outY = layout.contentY + layout.offsetY + (row * layout.cellH);
}

// ── Helper: convert world position to minimap pixel position ────────────────

static void worldToMinimap(const MinimapLayout &layout, int mapRows, int mapCols, float worldX, float worldZ,
                           float &outX, float &outY) {
    // Compute the world extent of the map using tileCenter for the last row/col.
    // Use the farthest tile centers to define the world coordinate range.
    Vector3 lastColCenter = hex::tileCenter(ODD_ROW_INDEX, mapCols - 1); // odd row for max X (includes offset)
    Vector3 lastRowCenter = hex::tileCenter(mapRows - 1, 0);

    float maxWorldX = lastColCenter.x;
    float maxWorldZ = lastRowCenter.z;

    // Map total pixel area.
    auto totalMapW = static_cast<float>(mapCols * layout.cellW);
    auto totalMapH = static_cast<float>(mapRows * layout.cellH);

    // Normalize world position to [0, 1].
    float normX = (maxWorldX > 0.0F) ? (worldX / maxWorldX) : HALF;
    float normZ = (maxWorldZ > 0.0F) ? (worldZ / maxWorldZ) : HALF;

    outX = static_cast<float>(layout.contentX + layout.offsetX) + (normX * totalMapW);
    outY = static_cast<float>(layout.contentY + layout.offsetY) + (normZ * totalMapH);
}

// ── Helper: intersect a ray with the Y=0 ground plane ───────────────────────

static Vector2 intersectGroundPlane(Ray ray) {
    if (std::abs(ray.direction.y) < RAY_EPSILON) {
        // Ray is parallel to ground; return ray origin's XZ.
        return {.x = ray.position.x, .y = ray.position.z};
    }
    float t = -ray.position.y / ray.direction.y;
    return {
        .x = ray.position.x + (t * ray.direction.x),
        .y = ray.position.z + (t * ray.direction.z),
    };
}

// ── Helper: draw terrain tiles on the minimap ───────────────────────────────

static void drawTerrain(const game::Map &map, const game::FogOfWar *fog, game::FactionId playerFactionId,
                        const MinimapLayout &layout, int mapRows, int mapCols) {
    for (int row = 0; row < mapRows; ++row) {
        for (int col = 0; col < mapCols; ++col) {
            int cellX = 0;
            int cellY = 0;
            tileToMinimap(layout, row, col, cellX, cellY);

            // Determine fog state.
            game::VisibilityState visibility = game::VisibilityState::Visible;
            if (fog != nullptr) {
                visibility = fog->getVisibility(playerFactionId, row, col);
            }

            if (visibility == game::VisibilityState::Unexplored) {
                DrawRectangle(cellX, cellY, layout.cellW, layout.cellH, UNEXPLORED_COLOR);
                continue;
            }

            Color terrainColor = terrain_colors::terrainFillColor(map.tile(row, col).terrainType());

            if (visibility == game::VisibilityState::Explored) {
                terrainColor = dimColor(terrainColor);
            }

            DrawRectangle(cellX, cellY, layout.cellW, layout.cellH, terrainColor);
        }
    }
}

// ── Helper: draw city markers on the minimap ────────────────────────────────

static void drawCities(const std::vector<game::City> &cities, const game::FogOfWar *fog,
                       game::FactionId playerFactionId, const game::FactionRegistry &factionRegistry,
                       const MinimapLayout &layout) {
    for (const auto &city : cities) {
        int cityRow = city.centerRow();
        int cityCol = city.centerCol();

        // Only draw cities in visible or explored tiles.
        if (fog != nullptr) {
            auto vis = fog->getVisibility(playerFactionId, cityRow, cityCol);
            if (vis == game::VisibilityState::Unexplored) {
                continue;
            }
        }

        int cellX = 0;
        int cellY = 0;
        tileToMinimap(layout, cityRow, cityCol, cellX, cellY);

        // Look up faction color.
        auto markerColor = WHITE;
        auto cityFactionId = static_cast<game::FactionId>(city.factionId());
        const auto *faction = factionRegistry.findFaction(cityFactionId);
        if (faction != nullptr) {
            markerColor = faction_colors::factionColor(faction->type(), faction->colorIndex());
        }

        // Draw city as a larger rectangle centered on the cell.
        int markerX = cellX + ((layout.cellW - CITY_MARKER_SIZE) / 2);
        int markerY = cellY + ((layout.cellH - CITY_MARKER_SIZE) / 2);
        DrawRectangle(markerX, markerY, CITY_MARKER_SIZE, CITY_MARKER_SIZE, markerColor);
    }
}

// ── Helper: draw unit dots on the minimap ───────────────────────────────────

static void drawUnits(const std::vector<std::unique_ptr<game::Unit>> &units, const game::FogOfWar *fog,
                      game::FactionId playerFactionId, const game::FactionRegistry &factionRegistry,
                      const MinimapLayout &layout) {
    for (const auto &unit : units) {
        if (!unit->isAlive()) {
            continue;
        }

        int unitRow = unit->row();
        int unitCol = unit->col();

        // Only draw units in visible tiles (not explored-only).
        if (fog != nullptr) {
            auto vis = fog->getVisibility(playerFactionId, unitRow, unitCol);
            if (vis != game::VisibilityState::Visible) {
                continue;
            }
        }

        int cellX = 0;
        int cellY = 0;
        tileToMinimap(layout, unitRow, unitCol, cellX, cellY);

        // Look up faction color.
        auto dotColor = WHITE;
        const auto *faction = factionRegistry.findFaction(unit->factionId());
        if (faction != nullptr) {
            dotColor = faction_colors::factionColor(faction->type(), faction->colorIndex());
        }

        // Draw unit as a small dot centered on the cell.
        int dotX = cellX + ((layout.cellW - UNIT_DOT_SIZE) / 2);
        int dotY = cellY + ((layout.cellH - UNIT_DOT_SIZE) / 2);
        DrawRectangle(dotX, dotY, UNIT_DOT_SIZE, UNIT_DOT_SIZE, dotColor);
    }
}

// ── Helper: draw camera viewport indicator ──────────────────────────────────

static void drawViewport(Camera3D camera, const MinimapLayout &layout, int mapRows, int mapCols, int screenWidth,
                         int screenHeight) {
    // Cast rays from screen corners and find where they hit Y=0.
    Ray topLeftRay = GetScreenToWorldRay({.x = 0.0F, .y = 0.0F}, camera);
    Ray topRightRay = GetScreenToWorldRay({.x = static_cast<float>(screenWidth), .y = 0.0F}, camera);
    Ray bottomLeftRay = GetScreenToWorldRay({.x = 0.0F, .y = static_cast<float>(screenHeight)}, camera);
    Ray bottomRightRay =
        GetScreenToWorldRay({.x = static_cast<float>(screenWidth), .y = static_cast<float>(screenHeight)}, camera);

    Vector2 worldTL = intersectGroundPlane(topLeftRay);
    Vector2 worldTR = intersectGroundPlane(topRightRay);
    Vector2 worldBL = intersectGroundPlane(bottomLeftRay);
    Vector2 worldBR = intersectGroundPlane(bottomRightRay);

    // Convert world corners to minimap coordinates.
    float mmTLx = 0.0F;
    float mmTLy = 0.0F;
    float mmTRx = 0.0F;
    float mmTRy = 0.0F;
    float mmBLx = 0.0F;
    float mmBLy = 0.0F;
    float mmBRx = 0.0F;
    float mmBRy = 0.0F;

    worldToMinimap(layout, mapRows, mapCols, worldTL.x, worldTL.y, mmTLx, mmTLy);
    worldToMinimap(layout, mapRows, mapCols, worldTR.x, worldTR.y, mmTRx, mmTRy);
    worldToMinimap(layout, mapRows, mapCols, worldBL.x, worldBL.y, mmBLx, mmBLy);
    worldToMinimap(layout, mapRows, mapCols, worldBR.x, worldBR.y, mmBRx, mmBRy);

    // Draw the viewport as four lines forming a quadrilateral.
    DrawLine(static_cast<int>(mmTLx), static_cast<int>(mmTLy), static_cast<int>(mmTRx), static_cast<int>(mmTRy),
             VIEWPORT_COLOR);
    DrawLine(static_cast<int>(mmTRx), static_cast<int>(mmTRy), static_cast<int>(mmBRx), static_cast<int>(mmBRy),
             VIEWPORT_COLOR);
    DrawLine(static_cast<int>(mmBRx), static_cast<int>(mmBRy), static_cast<int>(mmBLx), static_cast<int>(mmBLy),
             VIEWPORT_COLOR);
    DrawLine(static_cast<int>(mmBLx), static_cast<int>(mmBLy), static_cast<int>(mmTLx), static_cast<int>(mmTLy),
             VIEWPORT_COLOR);
}

// ── Main draw function ──────────────────────────────────────────────────────

void draw(const game::Map &map, const game::FogOfWar *fog, game::FactionId playerFactionId,
          const std::vector<std::unique_ptr<game::Unit>> &units, const std::vector<game::City> &cities,
          const game::FactionRegistry &factionRegistry, Camera3D camera, int screenWidth, int screenHeight) {

    int mapRows = map.height();
    int mapCols = map.width();

    // Draw panel background and border.
    int panelX = screenWidth - WIDTH - MARGIN;
    int panelY = screenHeight - HEIGHT - MARGIN;
    DrawRectangle(panelX, panelY, WIDTH, HEIGHT, BACKGROUND_COLOR);
    DrawRectangleLines(panelX, panelY, WIDTH, HEIGHT, BORDER_COLOR);

    // Compute layout.
    MinimapLayout layout = computeLayout(screenWidth, screenHeight, mapRows, mapCols);

    // Draw all layers.
    drawTerrain(map, fog, playerFactionId, layout, mapRows, mapCols);
    drawCities(cities, fog, playerFactionId, factionRegistry, layout);
    drawUnits(units, fog, playerFactionId, factionRegistry, layout);
    drawViewport(camera, layout, mapRows, mapCols, screenWidth, screenHeight);
}

} // namespace engine::minimap
