#include "game/Map.h"

#include <array>
#include <cassert>
#include <random>

namespace game {

namespace {

constexpr int WATER_ELEVATION = 0;
constexpr int PLAINS_ELEVATION = 1;
constexpr int DESERT_ELEVATION = 1;
constexpr int SWAMP_ELEVATION = 1;
constexpr int FOREST_ELEVATION = 1;
constexpr int HILLS_ELEVATION = 2;
constexpr int MOUNTAIN_ELEVATION = 3;

constexpr int defaultElevationForTerrain(TerrainType terrain) {
    switch (terrain) {
    case TerrainType::Plains:
        return PLAINS_ELEVATION;
    case TerrainType::Hills:
        return HILLS_ELEVATION;
    case TerrainType::Forest:
        return FOREST_ELEVATION;
    case TerrainType::Water:
        return WATER_ELEVATION;
    case TerrainType::Mountain:
        return MOUNTAIN_ELEVATION;
    case TerrainType::Desert:
        return DESERT_ELEVATION;
    case TerrainType::Swamp:
        return SWAMP_ELEVATION;
    }
    return PLAINS_ELEVATION;
}

constexpr std::array<TerrainType, TERRAIN_TYPE_COUNT> allTerrains = {
    TerrainType::Plains,   TerrainType::Hills,  TerrainType::Forest, TerrainType::Water,
    TerrainType::Mountain, TerrainType::Desert, TerrainType::Swamp,
};

std::vector<std::vector<Tile>> generateTiles(int height, int width, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<std::size_t> dist(0, allTerrains.size() - 1);

    std::vector<std::vector<Tile>> tiles;
    tiles.reserve(height);
    for (int row = 0; row < height; ++row) {
        std::vector<Tile> rowTiles;
        rowTiles.reserve(width);
        for (int col = 0; col < width; ++col) {
            auto terrain = allTerrains.at(dist(rng));
            rowTiles.emplace_back(row, col, terrain);
            rowTiles.back().setElevation(defaultElevationForTerrain(terrain));
        }
        tiles.push_back(std::move(rowTiles));
    }
    return tiles;
}

} // namespace

Map::Map(int height, int width) : Map(height, width, std::random_device{}()) {}

Map::Map(int height, int width, std::uint64_t seed)
    : width_(width), height_(height), tiles_(generateTiles(height, width, seed)) {
    assert(width > 0);
    assert(height > 0);
}

const Tile &Map::tile(int row, int col) const {
    assert(row >= 0 && row < height_);
    assert(col >= 0 && col < width_);
    return tiles_[row][col];
}

Tile &Map::tile(int row, int col) {
    assert(row >= 0 && row < height_);
    assert(col >= 0 && col < width_);
    return tiles_[row][col];
}

int Map::width() const { return width_; }

int Map::height() const { return height_; }

} // namespace game
