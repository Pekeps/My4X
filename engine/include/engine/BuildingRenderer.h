#pragma once
#include "game/Building.h"
#include "game/FogOfWar.h"
#include <vector>
namespace engine {
/// Draw buildings on the map.
/// fog / playerFactionId: when fog is non-null, buildings on unexplored tiles
/// are hidden and buildings on explored tiles are rendered (they are terrain
/// improvements).
void drawBuildings(const std::vector<game::Building> &buildings, const game::FogOfWar *fog = nullptr,
                   game::FactionId playerFactionId = 0);
} // namespace engine
