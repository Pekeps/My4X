#pragma once
#include "game/City.h"
#include "game/DiplomacyManager.h"
#include "game/FactionRegistry.h"
#include <optional>
#include <vector>
namespace engine {
void drawCities(const std::vector<game::City> &cities, const game::FactionRegistry &factions,
                std::optional<game::CityId> selectedCityId = std::nullopt, game::FactionId playerFactionId = 0,
                const game::DiplomacyManager *diplomacy = nullptr);
} // namespace engine
