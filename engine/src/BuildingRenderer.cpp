#include "engine/BuildingRenderer.h"

#include "engine/HexGrid.h"
#include "game/Building.h"

#include "raylib.h"

namespace engine {

const float BUILDING_Y_OFFSET = 0.1F;
const float BUILDING_RADIUS = 0.2F;
const float BUILDING_HEIGHT = 0.15F;
const int BUILDING_SLICES = 6;

void drawBuildings(const std::vector<game::Building> &buildings) {
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
            Vector3 pos = hex::tileCenter(tile.row, tile.col);
            pos.y = BUILDING_Y_OFFSET;
            DrawCylinder(pos, BUILDING_RADIUS, BUILDING_RADIUS, BUILDING_HEIGHT, BUILDING_SLICES,
                         color);
        }
    }
}

} // namespace engine
