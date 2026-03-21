#pragma once

#include "engine/ModelManager.h"
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
#include "game/FogOfWar.h"
#include "game/Unit.h"

#include <memory>
#include <vector>

namespace engine {

// Draws units as 3D models on the hex grid.
// Uses the faction registry to look up each unit's faction color.
// models: the ModelManager used to look up each unit type's 3D model.
// selectedIndex: index of the selected unit (-1 for none), drawn with a highlight.
// playerFactionId: the human player's faction ID, used to draw diplomacy rings.
// diplomacy: if provided, draws colored rings to indicate diplomatic relation to the player.
// fog: if provided, enemy units on non-Visible tiles are hidden.
void drawUnits(const std::vector<std::unique_ptr<game::Unit>> &units, const game::FactionRegistry &factions,
               const ModelManager &models, int selectedIndex = -1, game::FactionId playerFactionId = 0,
               const game::DiplomacyManager *diplomacy = nullptr, const game::FogOfWar *fog = nullptr);

} // namespace engine
