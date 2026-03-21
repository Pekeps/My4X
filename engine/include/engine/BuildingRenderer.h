#pragma once
#include "engine/ModelManager.h"
#include "game/Building.h"
#include "game/FactionRegistry.h"
#include "game/FogOfWar.h"
#include <vector>
namespace engine {

/// Register procedural fallback 3D models for each building type.
/// Must be called after the raylib window is initialised.
void registerBuildingModels(ModelManager &models);

/// Draw buildings on the map.
/// models: the ModelManager used to look up each building type's 3D model.
/// factions: registry used to resolve faction colors for tinting.
/// fog / playerFactionId: when fog is non-null, buildings on unexplored tiles
/// are hidden and buildings on explored tiles are rendered (they are terrain
/// improvements).
void drawBuildings(const std::vector<game::Building> &buildings, const ModelManager &models,
                   const game::FactionRegistry &factions, const game::FogOfWar *fog = nullptr,
                   game::FactionId playerFactionId = 0);

} // namespace engine
