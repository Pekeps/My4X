#include "engine/CityRenderer.h"

#include "HexDraw.h"
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

void drawCities(const std::vector<game::City> &cities, const game::FactionRegistry &factions,
                std::optional<game::CityId> selectedCityId, game::FactionId playerFactionId,
                const game::DiplomacyManager *diplomacy, const game::FogOfWar *fog) {
    for (const auto &city : cities) {
        // Hide enemy cities on non-Visible tiles (fog of war).
        auto cityFactionIdVal = static_cast<game::FactionId>(city.factionId());
        if (fog != nullptr && cityFactionIdVal != playerFactionId) {
            auto vis = fog->getVisibility(playerFactionId, city.centerRow(), city.centerCol());
            if (vis != game::VisibilityState::Visible) {
                continue;
            }
        }

        const bool selected = selectedCityId && city.id() == *selectedCityId;

        // Default to GOLD marker; faction lookup may override both fill and marker.
        Color fillColor = faction_colors::cityTerritoryColor(game::FactionType::Player, 0, selected);
        Color markerColor = selected ? YELLOW : GOLD;

        const auto *faction = factions.findFaction(static_cast<game::FactionId>(city.factionId()));
        if (faction != nullptr) {
            fillColor = faction_colors::cityTerritoryColor(faction->type(), faction->colorIndex(), selected);
            if (!selected) {
                markerColor = faction_colors::factionColor(faction->type(), faction->colorIndex());
            }
            // When selected, keep YELLOW for the marker so it stays distinguishable.
        }

        for (const auto &tile : city.tiles()) {
            Vector3 center = hex::tileCenter(tile.first, tile.second);
            drawFilledHex3D(center, fillColor);
        }

        Vector3 markerPos = hex::tileCenter(city.centerRow(), city.centerCol());
        markerPos.y = CITY_MARKER_Y;
        DrawCylinder(markerPos, CITY_MARKER_RADIUS, CITY_MARKER_RADIUS, CITY_MARKER_HEIGHT, CITY_MARKER_SLICES,
                     markerColor);

        // Draw diplomacy indicator ring on foreign cities.
        auto cityFactionId = static_cast<game::FactionId>(city.factionId());
        if (diplomacy != nullptr && cityFactionId != playerFactionId) {
            auto relation = diplomacy->getRelation(playerFactionId, cityFactionId);
            Color dipColor = diplomacy_colors::diplomacyColor(relation);
            Vector3 ringPos = hex::tileCenter(city.centerRow(), city.centerCol());
            DrawCylinder(ringPos, diplomacy_colors::DIPLOMACY_RING_RADIUS, diplomacy_colors::DIPLOMACY_RING_RADIUS,
                         diplomacy_colors::DIPLOMACY_RING_HEIGHT, diplomacy_colors::DIPLOMACY_RING_SLICES, dipColor);
        }
    }
}

} // namespace engine
