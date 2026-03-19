#include "engine/CityRenderer.h"

#include "HexDraw.h"
#include "engine/HexGrid.h"
#include "game/City.h"

#include "raylib.h"

namespace engine {

const float CITY_MARKER_Y = 0.3F;
const float CITY_MARKER_RADIUS = 0.25F;
const float CITY_MARKER_HEIGHT = 0.5F;
const int CITY_MARKER_SLICES = 8;

void drawCities(const std::vector<game::City> &cities, std::optional<game::CityId> selectedCityId) {
    for (const auto &city : cities) {
        const bool selected = selectedCityId && city.id() == *selectedCityId;
        const Color fillColor = selected ? Color{120, 120, 255, 180} : Color{80, 80, 200, 120};
        const Color markerColor = selected ? YELLOW : GOLD;

        for (const auto &tile : city.tiles()) {
            Vector3 center = hex::tileCenter(tile.first, tile.second);
            drawFilledHex3D(center, fillColor);
        }

        Vector3 markerPos = hex::tileCenter(city.centerRow(), city.centerCol());
        markerPos.y = CITY_MARKER_Y;
        DrawCylinder(markerPos, CITY_MARKER_RADIUS, CITY_MARKER_RADIUS, CITY_MARKER_HEIGHT, CITY_MARKER_SLICES,
                     markerColor);
    }
}

} // namespace engine
