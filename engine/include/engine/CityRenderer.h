#pragma once
#include "game/City.h"
#include <optional>
#include <vector>
namespace engine {
void drawCities(const std::vector<game::City> &cities, std::optional<game::CityId> selectedCityId = std::nullopt);
} // namespace engine
