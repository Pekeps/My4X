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

/// Minimum map dimension required for demo rivers.
constexpr int RIVER_MIN_MAP_SIZE = 15;

/// Elevation used for river source tiles (hills).
constexpr int RIVER_SOURCE_ELEVATION = 3;

/// Elevation used for mid-river tiles.
constexpr int RIVER_MID_ELEVATION = 2;

/// Elevation used for lower-river tiles.
constexpr int RIVER_LOW_ELEVATION = 1;

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
            if (terrain == TerrainType::Water) {
                rowTiles.back().setWaterLevel(1);
            }
        }
        tiles.push_back(std::move(rowTiles));
    }
    return tiles;
}

/// Set up a connected river on a tile and its neighbor (outgoing on source, incoming on dest).
void setRiverSegment(std::vector<std::vector<Tile>> &tiles, int srcRow, int srcCol, HexDirection outDir, int mapHeight,
                     int mapWidth) {
    auto neighborOpt = neighborCoord(srcRow, srcCol, outDir, mapHeight, mapWidth);
    if (!neighborOpt) {
        return;
    }
    auto [nr, nc] = *neighborOpt;
    tiles.at(srcRow).at(srcCol).setOutgoingRiver(outDir);
    tiles.at(nr).at(nc).setIncomingRiver(oppositeDirection(outDir));
}

/// Add demo rivers to the generated map for visual testing.
/// Creates a chain of river tiles flowing downhill through the center of the map.
void addDemoRivers(std::vector<std::vector<Tile>> &tiles, int height, int width) {
    if (height < RIVER_MIN_MAP_SIZE || width < RIVER_MIN_MAP_SIZE) {
        return; // Map too small for demo rivers.
    }

    // River chain 1: flows from (5,10) heading SE then E across several tiles.
    // Force terrain to non-water with descending elevation.
    constexpr int R1_START_ROW = 5;
    constexpr int R1_START_COL = 10;

    // Tile 0: river source (hills, elevation 3)
    tiles.at(R1_START_ROW).at(R1_START_COL).setTerrainType(TerrainType::Hills);
    tiles.at(R1_START_ROW).at(R1_START_COL).setElevation(RIVER_SOURCE_ELEVATION);
    tiles.at(R1_START_ROW).at(R1_START_COL).setWaterLevel(0);

    // Segment 0->1: SE
    setRiverSegment(tiles, R1_START_ROW, R1_START_COL, HexDirection::SE, height, width);
    auto n1 = neighborCoord(R1_START_ROW, R1_START_COL, HexDirection::SE, height, width);
    if (n1) {
        auto [r1, c1] = *n1;
        tiles.at(r1).at(c1).setTerrainType(TerrainType::Plains);
        tiles.at(r1).at(c1).setElevation(RIVER_MID_ELEVATION);
        tiles.at(r1).at(c1).setWaterLevel(0);

        // Segment 1->2: E
        setRiverSegment(tiles, r1, c1, HexDirection::E, height, width);
        auto n2 = neighborCoord(r1, c1, HexDirection::E, height, width);
        if (n2) {
            auto [r2, c2] = *n2;
            tiles.at(r2).at(c2).setTerrainType(TerrainType::Plains);
            tiles.at(r2).at(c2).setElevation(RIVER_MID_ELEVATION);
            tiles.at(r2).at(c2).setWaterLevel(0);

            // Segment 2->3: SE
            setRiverSegment(tiles, r2, c2, HexDirection::SE, height, width);
            auto n3 = neighborCoord(r2, c2, HexDirection::SE, height, width);
            if (n3) {
                auto [r3, c3] = *n3;
                tiles.at(r3).at(c3).setTerrainType(TerrainType::Plains);
                tiles.at(r3).at(c3).setElevation(RIVER_LOW_ELEVATION);
                tiles.at(r3).at(c3).setWaterLevel(0);

                // Segment 3->4: E
                setRiverSegment(tiles, r3, c3, HexDirection::E, height, width);
                auto n4 = neighborCoord(r3, c3, HexDirection::E, height, width);
                if (n4) {
                    auto [r4, c4] = *n4;
                    tiles.at(r4).at(c4).setTerrainType(TerrainType::Plains);
                    tiles.at(r4).at(c4).setElevation(RIVER_LOW_ELEVATION);
                    tiles.at(r4).at(c4).setWaterLevel(0);

                    // Segment 4->5: SE (river mouth)
                    setRiverSegment(tiles, r4, c4, HexDirection::SE, height, width);
                    auto n5 = neighborCoord(r4, c4, HexDirection::SE, height, width);
                    if (n5) {
                        auto [r5, c5] = *n5;
                        tiles.at(r5).at(c5).setTerrainType(TerrainType::Plains);
                        tiles.at(r5).at(c5).setElevation(RIVER_LOW_ELEVATION);
                        tiles.at(r5).at(c5).setWaterLevel(0);
                    }
                }
            }
        }
    }
}

} // namespace

Map::Map(int height, int width) : Map(height, width, std::random_device{}()) {}

Map::Map(int height, int width, std::uint64_t seed)
    : width_(width), height_(height), tiles_(generateTiles(height, width, seed)) {
    assert(width > 0);
    assert(height > 0);
    addDemoRivers(tiles_, height, width);
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
