#include "game/MapGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <ranges>

namespace game {

// ── Constructor ──────────────────────────────────────────────────────────────

MapGenerator::MapGenerator(int height, int width, int factionCount, std::uint64_t seed)
    : height_(height), width_(width), factionCount_(factionCount), seed_(seed) {}

// ── Hash-based noise ─────────────────────────────────────────────────────────

double MapGenerator::hashNoise(int x, int y, std::uint64_t noiseSeed) {
    // Splitmix64-style hash combining coordinates and seed.
    auto value = noiseSeed;
    value += static_cast<std::uint64_t>(x) * mapgen::COORD_HASH_X_PRIME;
    value += static_cast<std::uint64_t>(y) * mapgen::COORD_HASH_Y_PRIME;

    value ^= value >> mapgen::HASH_SHIFT_A;
    value *= mapgen::HASH_MIX_MULT_A;
    value ^= value >> mapgen::HASH_SHIFT_B;
    value *= mapgen::HASH_MIX_MULT_B;
    value ^= value >> mapgen::HASH_SHIFT_C;

    // Map to [-1, 1).
    return (static_cast<double>(value) * mapgen::HASH_TO_DOUBLE_SCALE * mapgen::NOISE_RANGE_SCALE) +
           mapgen::NOISE_CENTER_OFFSET;
}

double MapGenerator::fractalNoise(int x, int y, std::uint64_t noiseSeed) {
    double total = 0.0;
    double amplitude = 1.0;
    double maxAmplitude = 0.0;

    for (int octave = 0; octave < mapgen::NOISE_OCTAVES; ++octave) {
        // Scale coordinates by frequency for this octave.
        auto freq = mapgen::BASE_FREQUENCY * std::pow(mapgen::LACUNARITY, octave);
        auto sampleX = static_cast<int>(std::round(static_cast<double>(x) * freq));
        auto sampleY = static_cast<int>(std::round(static_cast<double>(y) * freq));

        total += hashNoise(sampleX, sampleY, noiseSeed + static_cast<std::uint64_t>(octave)) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= mapgen::PERSISTENCE;
    }

    return total / maxAmplitude;
}

// ── Terrain classification ───────────────────────────────────────────────────

TerrainType MapGenerator::classifyTerrain(double elevation, double moisture) {
    // Deep low elevation -> Water
    if (elevation < mapgen::WATER_THRESHOLD) {
        return TerrainType::Water;
    }

    // High elevation -> Mountain
    if (elevation > mapgen::MOUNTAIN_THRESHOLD) {
        return TerrainType::Mountain;
    }

    // Upper-mid elevation -> Hills
    if (elevation > mapgen::HILLS_THRESHOLD) {
        return TerrainType::Hills;
    }

    // Low-to-mid elevation: use moisture to differentiate
    if (moisture < mapgen::DESERT_MOISTURE_THRESHOLD) {
        return TerrainType::Desert;
    }
    if (moisture > mapgen::SWAMP_MOISTURE_THRESHOLD) {
        return TerrainType::Swamp;
    }
    if (moisture > mapgen::FOREST_MOISTURE_THRESHOLD) {
        return TerrainType::Forest;
    }

    return TerrainType::Plains;
}

// ── Smoothing helpers ────────────────────────────────────────────────────────

namespace {

constexpr int DIRECTION_COUNT = 8;
constexpr std::array<std::pair<int, int>, DIRECTION_COUNT> DIRECTIONS = {{
    {-1, -1},
    {-1, 0},
    {-1, 1},
    {0, -1},
    {0, 1},
    {1, -1},
    {1, 0},
    {1, 1},
}};

/// Count how many of the 8 neighbours share the same terrain type as the
/// centre tile, and also return the total number of valid neighbours.
std::pair<int, int> countSameNeighbours(const std::vector<std::vector<TerrainType>> &snapshot, int row, int col,
                                        int height, int width) {
    auto current = snapshot.at(row).at(col);
    int sameCount = 0;
    int neighbourCount = 0;

    for (const auto &[dr, dc] : DIRECTIONS) {
        int nr = row + dr;
        int nc = col + dc;
        if (nr >= 0 && nr < height && nc >= 0 && nc < width) {
            ++neighbourCount;
            if (snapshot.at(nr).at(nc) == current) {
                ++sameCount;
            }
        }
    }
    return {sameCount, neighbourCount};
}

/// Find the most common terrain type among the 8 neighbours.
TerrainType mostCommonNeighbour(const std::vector<std::vector<TerrainType>> &snapshot, int row, int col, int height,
                                int width) {
    std::array<int, TERRAIN_TYPE_COUNT> counts = {};
    for (const auto &[dr, dc] : DIRECTIONS) {
        int nr = row + dr;
        int nc = col + dc;
        if (nr >= 0 && nr < height && nc >= 0 && nc < width) {
            counts.at(static_cast<std::size_t>(snapshot.at(nr).at(nc)))++;
        }
    }

    auto maxIt = std::ranges::max_element(counts); // NOLINT(readability-qualified-auto)
    return static_cast<TerrainType>(std::distance(counts.begin(), maxIt));
}

} // namespace

// ── Smoothing ────────────────────────────────────────────────────────────────

void MapGenerator::smoothMap(Map &map, int passes) {
    for (int pass = 0; pass < passes; ++pass) {
        // Build a snapshot of terrain types before this pass.
        std::vector<std::vector<TerrainType>> snapshot(map.height(), std::vector<TerrainType>(map.width()));
        for (int row = 0; row < map.height(); ++row) {
            for (int col = 0; col < map.width(); ++col) {
                snapshot.at(row).at(col) = map.tile(row, col).terrainType();
            }
        }

        for (int row = 0; row < map.height(); ++row) {
            for (int col = 0; col < map.width(); ++col) {
                auto [sameCount, neighbourCount] = countSameNeighbours(snapshot, row, col, map.height(), map.width());

                if (sameCount < mapgen::SMOOTHING_THRESHOLD && neighbourCount > 0) {
                    map.tile(row, col).setTerrainType(
                        mostCommonNeighbour(snapshot, row, col, map.height(), map.width()));
                }
            }
        }
    }
}

// ── Spawn point selection ────────────────────────────────────────────────────

std::vector<SpawnPoint> MapGenerator::findSpawnPoints(const Map &map) const {
    // Collect all passable tiles away from the border.
    std::vector<SpawnPoint> candidates;
    for (int row = mapgen::SPAWN_BORDER_MARGIN; row < map.height() - mapgen::SPAWN_BORDER_MARGIN; ++row) {
        for (int col = mapgen::SPAWN_BORDER_MARGIN; col < map.width() - mapgen::SPAWN_BORDER_MARGIN; ++col) {
            auto terrain = map.tile(row, col).terrainType();
            if (getTerrainProperties(terrain).passable) {
                candidates.emplace_back(row, col);
            }
        }
    }

    if (candidates.empty() || factionCount_ <= 0) {
        return {};
    }

    std::vector<SpawnPoint> spawns;
    spawns.reserve(factionCount_);

    // Pick spawns greedily, maximising minimum distance to all prior spawns.
    auto centreRow = map.height() / 2;
    auto centreCol = map.width() / 2;

    auto distToCenter = [centreRow, centreCol](const SpawnPoint &point) {
        int dr = point.first - centreRow;
        int dc = point.second - centreCol;
        return (dr * dr) + (dc * dc);
    };

    // First spawn: closest to center for a stable starting point.
    auto firstIt = std::ranges::min_element( // NOLINT(readability-qualified-auto)
        candidates,
        [&](const SpawnPoint &lhs, const SpawnPoint &rhs) { return distToCenter(lhs) < distToCenter(rhs); });
    spawns.push_back(*firstIt);

    // Subsequent spawns: farthest from all existing spawns.
    for (int i = 1; i < factionCount_; ++i) {
        int bestMinDist = -1;
        SpawnPoint bestCandidate = candidates.front();

        for (const auto &candidate : candidates) {
            int minDist = std::numeric_limits<int>::max();
            for (const auto &existing : spawns) {
                int dr = candidate.first - existing.first;
                int dc = candidate.second - existing.second;
                int dist = std::max(std::abs(dr), std::abs(dc)); // Chebyshev
                minDist = std::min(minDist, dist);
            }
            if (minDist > bestMinDist) {
                bestMinDist = minDist;
                bestCandidate = candidate;
            }
        }

        spawns.push_back(bestCandidate);
    }

    return spawns;
}

// ── Public interface ─────────────────────────────────────────────────────────

Map MapGenerator::generate() const {
    // Create a blank map (default terrain = Plains).
    Map map(height_, width_);

    // Assign terrain from noise.
    for (int row = 0; row < height_; ++row) {
        for (int col = 0; col < width_; ++col) {
            double elevation = fractalNoise(row, col, seed_);
            double moisture = fractalNoise(row, col, seed_ + mapgen::MOISTURE_SEED_OFFSET);
            map.tile(row, col).setTerrainType(classifyTerrain(elevation, moisture));
        }
    }

    smoothMap(map, mapgen::SMOOTHING_PASSES);
    return map;
}

std::vector<SpawnPoint> MapGenerator::spawnPoints() const {
    auto map = generate();
    return findSpawnPoints(map);
}

} // namespace game
