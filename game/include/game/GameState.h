#pragma once

#include "game/Building.h"
#include "game/City.h"
#include "game/DiplomacyManager.h"
#include "game/Faction.h"
#include "game/FactionRegistry.h"
#include "game/FogOfWar.h"
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
    [[nodiscard]] Map &mutableMap();

    // -- TileRegistry --------------------------------------------------

    [[nodiscard]] const TileRegistry &registry() const;
    [[nodiscard]] TileRegistry &mutableRegistry();

    // -- Cities --------------------------------------------------------

    CityId addCity(City city);
    void removeCity(CityId id);
    [[nodiscard]] const std::vector<City> &cities() const;
    [[nodiscard]] std::vector<City> &mutableCities();
    [[nodiscard]] std::vector<City> &cities();

    /// Find a city by ID, or nullptr if not found.
    [[nodiscard]] City *findCity(CityId id);

    /// Return pointers to all cities owned by the given faction.
    [[nodiscard]] std::vector<const City *> citiesForFaction(int factionId) const;

    /// Return mutable pointers to all cities owned by the given faction.
    [[nodiscard]] std::vector<City *> mutableCitiesForFaction(int factionId);

    /// Add a city preserving its existing ID (for deserialization).
    void restoreCity(City city);
    void setNextCityId(CityId id);

    // -- Buildings -----------------------------------------------------

    BuildingId addBuilding(Building building);
    void removeBuilding(BuildingId id);
    [[nodiscard]] const std::vector<Building> &buildings() const;

    /// Add a building preserving its existing ID (for deserialization).
    void restoreBuilding(Building building);
    void setNextBuildingId(BuildingId id);

    // -- Units ---------------------------------------------------------

    /// Add a unit and register it in the TileRegistry. Returns the index.
    std::size_t addUnit(std::unique_ptr<Unit> unit);
    void removeUnit(std::size_t index);

    /// Remove all dead units (health <= 0) from the unit list and tile
    /// registry.  Returns the number of units removed.  If @p selectedIndex
    /// is not null, the pointed-to value is adjusted: cleared to -1 if the
    /// selected unit was removed, or decremented for each removed unit whose
    /// index was below the selected one.
    std::size_t removeDeadUnits(int *selectedIndex = nullptr);

    [[nodiscard]] const std::vector<std::unique_ptr<Unit>> &units() const;
    [[nodiscard]] std::vector<std::unique_ptr<Unit>> &units();

    /// Return pointers to all units owned by the given faction.
    [[nodiscard]] std::vector<const Unit *> unitsForFaction(FactionId factionId) const;

    // -- FactionRegistry -----------------------------------------------

    [[nodiscard]] const FactionRegistry &factionRegistry() const;
    [[nodiscard]] FactionRegistry &mutableFactionRegistry();

    // -- DiplomacyManager ----------------------------------------------

    [[nodiscard]] const DiplomacyManager &diplomacy() const;
    [[nodiscard]] DiplomacyManager &mutableDiplomacy();

    // -- Faction resources ---------------------------------------------

    [[nodiscard]] const Resource &factionResources() const;
    [[nodiscard]] Resource &factionResources();

    // -- Fog of War ----------------------------------------------------

    [[nodiscard]] const FogOfWar &fogOfWar() const;
    [[nodiscard]] FogOfWar &mutableFogOfWar();

    /// Recalculate fog of war visibility for the given faction based on
    /// current unit and city positions.
    void recalculateFog(FactionId factionId);

    void setTurn(int turn);
    [[nodiscard]] CityId nextCityId() const;
    [[nodiscard]] BuildingId nextBuildingId() const;

  private:
    int turn_ = 1;
    Map map_;
    TileRegistry registry_;
    FogOfWar fog_;
    std::vector<City> cities_;
    std::vector<Building> buildings_;
    std::vector<std::unique_ptr<Unit>> units_;
    FactionRegistry factionRegistry_;
    DiplomacyManager diplomacy_;
    Resource factionResources_;

    CityId nextCityId_ = 1;
    BuildingId nextBuildingId_ = 1;
};

} // namespace game
