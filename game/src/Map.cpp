#include "game/Map.h"

#include <array>
#include <cassert>
#include <random>

namespace game {

namespace {

constexpr std::array<TerrainType, 5> allTerrains = {
    TerrainType::Plains, TerrainType::Hills, TerrainType::Forest, TerrainType::Water, TerrainType::Mountain,
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
            rowTiles.emplace_back(row, col, allTerrains.at(dist(rng)));
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

int Map::width() const { return width_; }

int Map::height() const { return height_; }

} // namespace game
