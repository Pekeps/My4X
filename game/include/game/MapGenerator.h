#pragma once

#include "game/Map.h"
#include "game/TerrainType.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace game {

/// Named constants for map generation parameters.
namespace mapgen {

// ── Noise parameters ─────────────────────────────────────────────────────────

/// Number of octaves for fractal noise layering.
static constexpr int NOISE_OCTAVES = 4;

/// Base frequency for noise sampling (higher = more variation per tile).
static constexpr double BASE_FREQUENCY = 0.08;

/// How much each successive octave's frequency is multiplied.
static constexpr double LACUNARITY = 2.0;

/// How much each successive octave's amplitude is reduced.
static constexpr double PERSISTENCE = 0.5;

// ── Terrain distribution thresholds (cumulative, elevation-based) ────────────
//
// The generator uses two noise layers: elevation and moisture.
// Elevation determines the base terrain category, moisture refines it.
//
// Approximate target distribution:
//   Plains ~40%, Forest ~20%, Hills ~15%, Mountain ~10%,
//   Water ~10%, Desert ~3%, Swamp ~2%

/// Elevation below this threshold becomes Water (~10%).
static constexpr double WATER_THRESHOLD = -0.60;

/// Elevation above this threshold becomes Mountain (~10%).
static constexpr double MOUNTAIN_THRESHOLD = 0.60;

/// Elevation above this threshold (but below mountain) becomes Hills (~15%).
static constexpr double HILLS_THRESHOLD = 0.35;

/// Moisture below this threshold in low-mid elevation becomes Desert.
static constexpr double DESERT_MOISTURE_THRESHOLD = -0.55;

/// Moisture above this threshold in low-mid elevation becomes Swamp.
static constexpr double SWAMP_MOISTURE_THRESHOLD = 0.55;

/// Moisture above this threshold in mid elevation becomes Forest.
static constexpr double FOREST_MOISTURE_THRESHOLD = -0.10;

// ── Smoothing ────────────────────────────────────────────────────────────────

/// Number of cellular-automaton smoothing passes after initial generation.
static constexpr int SMOOTHING_PASSES = 2;

/// Minimum neighbours of the same terrain type needed to keep a tile during
/// smoothing (out of 8 neighbours + the tile itself = 9 total).
static constexpr int SMOOTHING_THRESHOLD = 4;

// ── Spawn placement ──────────────────────────────────────────────────────────

/// Minimum distance (Chebyshev) between any two spawn locations.
static constexpr int MIN_SPAWN_DISTANCE = 5;

/// Border margin: spawns will not be placed within this many tiles of the
/// map edge.
static constexpr int SPAWN_BORDER_MARGIN = 2;

// ── Hash-based noise constants ───────────────────────────────────────────────

/// Large prime used in the hash-based noise function.
static constexpr std::uint64_t HASH_PRIME_A = 6364136223846793005ULL;

/// Another large prime for mixing.
static constexpr std::uint64_t HASH_PRIME_B = 1442695040888963407ULL;

/// Bit-shift amount for hash mixing.
static constexpr int HASH_SHIFT_A = 30;

/// Second bit-shift amount.
static constexpr int HASH_SHIFT_B = 27;

/// Third bit-shift amount.
static constexpr int HASH_SHIFT_C = 31;

/// Multiplier for final hash mixing step.
static constexpr std::uint64_t HASH_MIX_MULT_A = 0xbf58476d1ce4e5b9ULL;

/// Multiplier for second mixing step.
static constexpr std::uint64_t HASH_MIX_MULT_B = 0x94d049bb133111ebULL;

/// Scale factor to convert uint64 hash output to [0, 1) double.
static constexpr double HASH_TO_DOUBLE_SCALE = 1.0 / static_cast<double>(UINT64_MAX);

/// Offset applied after scaling to centre noise around zero: [-1, 1).
static constexpr double NOISE_CENTER_OFFSET = -1.0;

/// Multiplier to expand [0,1) to [0,2) before centering.
static constexpr double NOISE_RANGE_SCALE = 2.0;

/// Seed offset between elevation and moisture noise layers so they are
/// uncorrelated.
static constexpr std::uint64_t MOISTURE_SEED_OFFSET = 1000;

// ── Coordinate hashing primes ────────────────────────────────────────────────

/// Prime multiplied with the x-coordinate in the hash function.
static constexpr std::uint64_t COORD_HASH_X_PRIME = 374761393ULL;

/// Prime multiplied with the y-coordinate in the hash function.
static constexpr std::uint64_t COORD_HASH_Y_PRIME = 668265263ULL;

} // namespace mapgen

/// A coordinate pair representing a spawn location on the map.
using SpawnPoint = std::pair<int, int>;

/// Procedural map generator.
///
/// Produces a fully populated Map using layered hash-based noise for elevation
/// and moisture, then maps the resulting values to terrain types.  The output
/// is fully deterministic for a given seed.
class MapGenerator {
  public:
    /// Create a generator with the given map dimensions, faction count, and
    /// random seed.
    MapGenerator(int height, int width, int factionCount, std::uint64_t seed);

    /// Generate and return the map.
    [[nodiscard]] Map generate() const;

    /// After generate() has been called, retrieve the computed spawn points.
    /// Each entry is a (row, col) pair on passable terrain, spread across the
    /// map for fairness.
    [[nodiscard]] std::vector<SpawnPoint> spawnPoints() const;

  private:
    int height_;
    int width_;
    int factionCount_;
    std::uint64_t seed_;

    /// Hash-based pseudo-random noise at integer coordinates.
    [[nodiscard]] static double hashNoise(int x, int y, std::uint64_t noiseSeed);

    /// Fractal (multi-octave) noise combining several hashNoise samples.
    [[nodiscard]] static double fractalNoise(int x, int y, std::uint64_t noiseSeed);

    /// Map elevation + moisture noise values to a TerrainType.
    [[nodiscard]] static TerrainType classifyTerrain(double elevation, double moisture);

    /// Apply smoothing passes to reduce noise speckle.
    static void smoothMap(Map &map, int passes);

    /// Find fair spawn points spread across the map on passable terrain.
    [[nodiscard]] std::vector<SpawnPoint> findSpawnPoints(const Map &map) const;
};

} // namespace game
