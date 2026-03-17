#pragma once

#include "game/BuildQueue.h"

#include <cstdint>
#include <set>
#include <string>
#include <utility>

namespace game {

using CityId = std::uint32_t;

class City {
  public:
    City(std::string name, int centerRow, int centerCol, int factionId = 0);

    [[nodiscard]] CityId id() const;
    void setId(CityId newId);

    [[nodiscard]] const std::string &name() const;
    [[nodiscard]] int centerRow() const;
    [[nodiscard]] int centerCol() const;
    [[nodiscard]] int factionId() const;
    [[nodiscard]] int population() const;

    void growPopulation(int amount);

    /// Expand the city footprint to include the given tile.
    /// Returns false if the tile is already part of this city.
    bool addTile(int row, int col);

    /// Returns the set of tile coordinates the city occupies.
    [[nodiscard]] const std::set<std::pair<int, int>> &tiles() const;

    /// Check whether the city occupies the given tile.
    [[nodiscard]] bool containsTile(int row, int col) const;

    [[nodiscard]] BuildQueue &buildQueue();
    [[nodiscard]] const BuildQueue &buildQueue() const;
    [[nodiscard]] static int productionPerTurn();

  private:
    CityId id_ = 0;
    std::string name_;
    int centerRow_;
    int centerCol_;
    int factionId_;
    int population_ = 1;
    std::set<std::pair<int, int>> tiles_;
    BuildQueue buildQueue_;
};

} // namespace game
