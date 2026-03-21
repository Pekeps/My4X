#pragma once

#include "engine/ModelManager.h"
#include "engine/UnitAnimator.h"
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
#include "game/FogOfWar.h"
#include "game/Map.h"
#include "game/Unit.h"

#include <memory>
#include <vector>

namespace engine {

// Draws units as 3D models on the hex grid.
// map: used for elevation-based Y positioning.
void drawUnits(const std::vector<std::unique_ptr<game::Unit>> &units, const game::FactionRegistry &factions,
               const ModelManager &models, const game::Map &map, int selectedIndex = -1,
               game::FactionId playerFactionId = 0, const game::DiplomacyManager *diplomacy = nullptr,
               const game::FogOfWar *fog = nullptr, const UnitAnimator *animator = nullptr);

} // namespace engine
