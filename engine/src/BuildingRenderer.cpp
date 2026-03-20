#include "engine/BuildingRenderer.h"

#include "engine/HexGrid.h"
#include "game/Building.h"

#include "raylib.h"

namespace engine {

const float BUILDING_Y_OFFSET = 0.1F;
const float BUILDING_RADIUS = 0.2F;
const float BUILDING_HEIGHT = 0.15F;
const int BUILDING_SLICES = 6;

/// Factor to dim building colors on explored-but-not-visible tiles.
static constexpr float BUILDING_DIM_FACTOR = 0.5F;

static Color dimBuildingColor(Color base) {
    return Color{
        .r = static_cast<unsigned char>(static_cast<float>(base.r) * BUILDING_DIM_FACTOR),
        .g = static_cast<unsigned char>(static_cast<float>(base.g) * BUILDING_DIM_FACTOR),
        .b = static_cast<unsigned char>(static_cast<float>(base.b) * BUILDING_DIM_FACTOR),
        .a = base.a,
    };
}

void drawBuildings(const std::vector<game::Building> &buildings, const game::FogOfWar *fog,
                   game::FactionId playerFactionId) {
    for (const auto &building : buildings) {
        auto color = GRAY;
        if (building.name() == "Farm") {
            color = GREEN;
        } else if (building.name() == "Mine") {
            color = BROWN;
        } else if (building.name() == "Market") {
            color = GOLD;
        }

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
            pos.y = BUILDING_Y_OFFSET;
            DrawCylinder(pos, BUILDING_RADIUS, BUILDING_RADIUS, BUILDING_HEIGHT, BUILDING_SLICES, color);
        }
    }
}

} // namespace engine
