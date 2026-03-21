#include "engine/CityRenderer.h"

#include "engine/DiplomacyColors.h"
#include "engine/FactionColors.h"
#include "engine/HexGrid.h"
#include "game/City.h"

#include "raylib.h"

namespace engine {

static constexpr float CITY_MARKER_Y = 0.3F;
static constexpr float CITY_MARKER_RADIUS = 0.25F;
static constexpr float CITY_MARKER_HEIGHT = 0.5F;
static constexpr int CITY_MARKER_SLICES = 8;

void drawCities(const std::vector<game::City> &cities, const game::FactionRegistry &factions, const game::Map &map,
                std::optional<game::CityId> selectedCityId, game::FactionId playerFactionId,
                const game::DiplomacyManager *diplomacy, const game::FogOfWar *fog) {
    for (const auto &city : cities) {
        auto cityFactionIdVal = static_cast<game::FactionId>(city.factionId());
        if (fog != nullptr && cityFactionIdVal != playerFactionId) {
            auto vis = fog->getVisibility(playerFactionId, city.centerRow(), city.centerCol());
            if (vis != game::VisibilityState::Visible) {
                continue;
            }
        }

        const bool selected = selectedCityId && city.id() == *selectedCityId;
        Color markerColor = selected ? YELLOW : GOLD;

        const auto *faction = factions.findFaction(static_cast<game::FactionId>(city.factionId()));
        if (faction != nullptr && !selected) {
            markerColor = faction_colors::factionColor(faction->type(), faction->colorIndex());
        }

        Vector3 markerPos = hex::tileCenterElevated(city.centerRow(), city.centerCol(), map);
        markerPos.y += CITY_MARKER_Y;
        DrawCylinder(markerPos, CITY_MARKER_RADIUS, CITY_MARKER_RADIUS, CITY_MARKER_HEIGHT, CITY_MARKER_SLICES,
                     markerColor);

        // Draw diplomacy indicator ring on foreign cities.
        auto cityFactionId = static_cast<game::FactionId>(city.factionId());
        if (diplomacy != nullptr && cityFactionId != playerFactionId) {
            auto relation = diplomacy->getRelation(playerFactionId, cityFactionId);
            Color dipColor = diplomacy_colors::diplomacyColor(relation);
            Vector3 ringPos = hex::tileCenterElevated(city.centerRow(), city.centerCol(), map);
            DrawCylinder(ringPos, diplomacy_colors::DIPLOMACY_RING_RADIUS, diplomacy_colors::DIPLOMACY_RING_RADIUS,
                         diplomacy_colors::DIPLOMACY_RING_HEIGHT, diplomacy_colors::DIPLOMACY_RING_SLICES, dipColor);
        }
    }
}

} // namespace engine
