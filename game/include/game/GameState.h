#pragma once

#include "game/Building.h"
#include "game/City.h"
#include "game/Map.h"
#include "game/Resource.h"
#include "game/TileRegistry.h"
#include "game/Unit.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace game {

/// Central game-state container.
///
/// Holds every piece of mutable game data: the map, entities (cities,
/// buildings, units), tile-occupancy registry, per-faction resources, and the
/// turn counter.  Logic lives elsewhere (TurnResolver, renderers, etc.) —
/// GameState is intentionally a data container.
class GameState {
  public:
    /// Construct a GameState that owns a Map of the given dimensions.
    GameState(int mapRows, int mapCols);

    /// Construct a GameState that owns a Map with a specific seed.
    GameState(int mapRows, int mapCols, std::uint64_t seed);

    // -- Turn ----------------------------------------------------------

    void nextTurn();
    [[nodiscard]] int getTurn() const;

    // -- Map -----------------------------------------------------------

    [[nodiscard]] const Map &map() const;

    // -- TileRegistry --------------------------------------------------

    [[nodiscard]] const TileRegistry &registry() const;

    // -- Cities --------------------------------------------------------

    CityId addCity(City city);
    void removeCity(CityId id);
    [[nodiscard]] const std::vector<City> &cities() const;

    // -- Buildings -----------------------------------------------------

    BuildingId addBuilding(Building building);
    void removeBuilding(BuildingId id);
    [[nodiscard]] const std::vector<Building> &buildings() const;

    // -- Units ---------------------------------------------------------

    /// Add a unit and register it in the TileRegistry. Returns the index.
    std::size_t addUnit(std::unique_ptr<Unit> unit);
    void removeUnit(std::size_t index);
    [[nodiscard]] const std::vector<std::unique_ptr<Unit>> &units() const;
    [[nodiscard]] std::vector<std::unique_ptr<Unit>> &units();

    // -- Faction resources ---------------------------------------------

    [[nodiscard]] const Resource &factionResources() const;
    [[nodiscard]] Resource &factionResources();

  private:
    int turn_ = 1;
    Map map_;
    TileRegistry registry_;
    std::vector<City> cities_;
    std::vector<Building> buildings_;
    std::vector<std::unique_ptr<Unit>> units_;
    Resource factionResources_;

    CityId nextCityId_ = 1;
    BuildingId nextBuildingId_ = 1;
};

} // namespace game
