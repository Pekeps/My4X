#pragma once

#include "game/Unit.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace game {

/// Unique identifier for a city entity.
using CityId = std::uint32_t;

/// Unique identifier for a building entity.
using BuildingId = std::uint32_t;

/// Spatial registry that tracks what occupies each map tile.
///
/// Provides O(1) lookups for city, building, and unit occupancy by (row, col).
/// Stores identifiers and pointers rather than copies of entities.
class TileRegistry {
  public:
    /// Register a city as occupying the given tile.
    void registerCity(int row, int col, CityId city);

    /// Remove a city from the given tile.
    void unregisterCity(int row, int col);

    /// Register a building as occupying the given tile.
    void registerBuilding(int row, int col, BuildingId building);

    /// Remove a building from the given tile.
    void unregisterBuilding(int row, int col);

    /// Register a unit as occupying the given tile.
    void registerUnit(int row, int col, Unit *unit);

    /// Remove a unit from the given tile.
    void unregisterUnit(int row, int col, Unit *unit);

    /// Return the city occupying the given tile, or std::nullopt if none.
    [[nodiscard]] std::optional<CityId> cityAt(int row, int col) const;

    /// Return the building on the given tile, or std::nullopt if none.
    [[nodiscard]] std::optional<BuildingId> buildingAt(int row, int col) const;

    /// Return all units on the given tile (may be empty).
    [[nodiscard]] std::vector<Unit *> unitsAt(int row, int col) const;

    /// Quick check: is there any entity (city, building, or unit) on this tile?
    [[nodiscard]] bool isOccupied(int row, int col) const;

  private:
    struct TileCoord {
        int row;
        int col;

        bool operator==(const TileCoord &other) const = default;
    };

    struct TileCoordHash {
        std::size_t operator()(const TileCoord &coord) const noexcept {
            // Combine row and col into a single hash using a standard technique.
            auto hash1 = std::hash<int>{}(coord.row);
            auto hash2 = std::hash<int>{}(coord.col);
            return hash1 ^ (hash2 << 1U);
        }
    };

    std::unordered_map<TileCoord, CityId, TileCoordHash> cities_;
    std::unordered_map<TileCoord, BuildingId, TileCoordHash> buildings_;
    std::unordered_map<TileCoord, std::vector<Unit *>, TileCoordHash> units_;
};

} // namespace game
