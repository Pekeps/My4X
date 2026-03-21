# Terrain Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transform flat-colored hex tiles into organic, blended, terraced terrain with Perlin noise irregularity (Milestone 1: TON-75 through TON-78).

**Architecture:** The work spans two layers: `game/` (pure data — hex directions, elevation) and `engine/` (rendering — mesh builder, blending, terraces, noise). A new `HexMeshBuilder` provides per-vertex-colored triangle rendering via rlgl, replacing `DrawTriangle3D`. A `HexDirection` enum in the game layer provides reusable direction/neighbor utilities shared by pathfinding, rendering, fog-of-war, and future systems (rivers, roads). Rendering logic currently embedded in `main.cpp` is not in scope for extraction here (main.cpp changes are minimal), but the new rendering modules are designed as clean, self-contained engine components.

**Tech Stack:** C++23, raylib 5.5 (rlgl for per-vertex colors), GoogleTest, CMake/Ninja

**Reference:** [Catlike Coding Hex Map](https://catlikecoding.com/unity/tutorials/hex-map/) Parts 2-4

---

## File Map

### New files

| File | Responsibility |
|------|---------------|
| `game/include/game/HexDirection.h` | `HexDirection` enum (NE, E, SE, SW, W, NW), neighbor coordinate offsets, opposite direction, iteration helpers |
| `tests/game/test_hex_direction.cpp` | Unit tests for direction utilities |
| `engine/include/engine/HexMeshBuilder.h` | Accumulates triangles with per-vertex positions + colors, flushes via rlgl |
| `engine/src/HexMeshBuilder.cpp` | Implementation |
| `engine/include/engine/HexMetrics.h` | Shared hex geometry constants: solid factor, blend factor, terrace steps, elevation step size |
| `engine/include/engine/EdgeVertices.h` | `EdgeVertices` struct: 4 vertices per subdivided edge, interpolation helpers |
| `engine/include/engine/HexEdgeType.h` | `HexEdgeType` enum (Flat, Slope, Cliff), classification function |
| `engine/include/engine/PerlinNoise.h` | Deterministic 2D Perlin noise sampler |
| `engine/src/PerlinNoise.cpp` | Implementation |
| `tests/engine/test_perlin_noise.cpp` | Tests for noise determinism and range |
| `tests/engine/test_hex_mesh_builder.cpp` | Tests for mesh builder vertex/color accumulation |

### Modified files

| File | Changes |
|------|---------|
| `game/include/game/Tile.h` | Add `int elevation_` field, getter/setter |
| `game/src/Tile.cpp` | Initialize elevation from terrain type |
| `game/src/Map.cpp` | Set default elevations during generation |
| `game/CMakeLists.txt` | No new .cpp files (HexDirection is header-only) |
| `engine/CMakeLists.txt` | Add `HexMeshBuilder.cpp`, `PerlinNoise.cpp` |
| `engine/src/MapRenderer.cpp` | Major rewrite: blended triangulation, terraces, edge subdivision, noise perturbation |
| `engine/src/HexDraw.h` | Add `drawFilledHex3DBlended` or deprecate in favor of HexMeshBuilder |
| `tests/CMakeLists.txt` | Add new test files |
| `tests/game/test_terrain.cpp` | Add elevation-related tests |

---

## Task 1: HexDirection enum and neighbor utilities

**Linear:** TON-75 (prerequisite)

This is the reusable foundation for direction-aware neighbor access. Currently, neighbor offsets are duplicated as local constants in `Pathfinding.cpp`, `ZoneOfControl.cpp`, `NeutralAI.cpp`, etc. This task centralizes them.

**Files:**
- Create: `game/include/game/HexDirection.h`
- Create: `tests/game/test_hex_direction.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write HexDirection.h**

```cpp
// game/include/game/HexDirection.h
#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace game {

/// Six hex directions for flat-top hexagons in odd-r offset coordinates.
/// Order matters: opposite direction is (dir + 3) % 6.
enum class HexDirection : std::uint8_t {
    NE = 0, // row-1, col+(odd row offset)
    E  = 1, // row+0, col+1
    SE = 2, // row+1, col+(odd row offset)
    SW = 3, // row+1, col-(even row offset)
    W  = 4, // row+0, col-1
    NW = 5, // row-1, col-(even row offset)
};

static constexpr int HEX_DIRECTION_COUNT = 6;

/// Return the opposite direction (180 degrees).
[[nodiscard]] constexpr HexDirection oppositeDirection(HexDirection dir) {
    static constexpr int OPPOSITE_OFFSET = 3;
    return static_cast<HexDirection>((static_cast<int>(dir) + OPPOSITE_OFFSET) % HEX_DIRECTION_COUNT);
}

/// Neighbor coordinate offsets for even rows (row % 2 == 0) in odd-r layout.
/// Order: NE, E, SE, SW, W, NW — matches HexDirection enum.
static constexpr std::array<std::array<int, 2>, HEX_DIRECTION_COUNT> EVEN_ROW_OFFSETS = {{
    {-1, 0},  // NE
    {0, 1},   // E
    {1, 0},   // SE
    {1, -1},  // SW
    {0, -1},  // W
    {-1, -1}, // NW
}};

/// Neighbor coordinate offsets for odd rows (row % 2 == 1) in odd-r layout.
static constexpr std::array<std::array<int, 2>, HEX_DIRECTION_COUNT> ODD_ROW_OFFSETS = {{
    {-1, 1}, // NE
    {0, 1},  // E
    {1, 1},  // SE
    {1, 0},  // SW
    {0, -1}, // W
    {-1, 0}, // NW
}};

/// Compute the neighbor coordinate in a given direction.
/// Returns nullopt if the neighbor is outside [0, mapHeight) x [0, mapWidth).
[[nodiscard]] inline std::optional<std::pair<int, int>> neighborCoord(
    int row, int col, HexDirection dir, int mapHeight, int mapWidth) {
    const auto &offsets = ((row & 1) == 0) ? EVEN_ROW_OFFSETS : ODD_ROW_OFFSETS;
    int idx = static_cast<int>(dir);
    int nr = row + offsets[idx][0];
    int nc = col + offsets[idx][1];
    if (nr < 0 || nr >= mapHeight || nc < 0 || nc >= mapWidth) {
        return std::nullopt;
    }
    return std::pair{nr, nc};
}

/// Iterate over all 6 directions.
static constexpr std::array<HexDirection, HEX_DIRECTION_COUNT> ALL_DIRECTIONS = {
    HexDirection::NE, HexDirection::E,  HexDirection::SE,
    HexDirection::SW, HexDirection::W,  HexDirection::NW,
};

} // namespace game
```

- [ ] **Step 2: Write tests**

```cpp
// tests/game/test_hex_direction.cpp
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/HexDirection.h"
#include <gtest/gtest.h>

TEST(HexDirectionTest, OppositeDirections) {
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::NE), game::HexDirection::SW);
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::E),  game::HexDirection::W);
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::SE), game::HexDirection::NW);
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::SW), game::HexDirection::NE);
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::W),  game::HexDirection::E);
    EXPECT_EQ(game::oppositeDirection(game::HexDirection::NW), game::HexDirection::SE);
}

TEST(HexDirectionTest, DoubleOppositeIsIdentity) {
    for (auto dir : game::ALL_DIRECTIONS) {
        EXPECT_EQ(game::oppositeDirection(game::oppositeDirection(dir)), dir);
    }
}

TEST(HexDirectionTest, NeighborCoordEvenRow) {
    // Row 0 col 2: even row
    auto ne = game::neighborCoord(0, 2, game::HexDirection::NE, 10, 10);
    EXPECT_FALSE(ne.has_value()); // row -1 is out of bounds

    auto e = game::neighborCoord(0, 2, game::HexDirection::E, 10, 10);
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(e->first, 0);
    EXPECT_EQ(e->second, 3);

    auto se = game::neighborCoord(0, 2, game::HexDirection::SE, 10, 10);
    ASSERT_TRUE(se.has_value());
    EXPECT_EQ(se->first, 1);
    EXPECT_EQ(se->second, 2);
}

TEST(HexDirectionTest, NeighborCoordOddRow) {
    // Row 1 col 2: odd row (shifted right)
    auto ne = game::neighborCoord(1, 2, game::HexDirection::NE, 10, 10);
    ASSERT_TRUE(ne.has_value());
    EXPECT_EQ(ne->first, 0);
    EXPECT_EQ(ne->second, 3);

    auto sw = game::neighborCoord(1, 2, game::HexDirection::SW, 10, 10);
    ASSERT_TRUE(sw.has_value());
    EXPECT_EQ(sw->first, 2);
    EXPECT_EQ(sw->second, 2);
}

TEST(HexDirectionTest, NeighborOutOfBounds) {
    auto result = game::neighborCoord(0, 0, game::HexDirection::W, 5, 5);
    EXPECT_FALSE(result.has_value());
}

TEST(HexDirectionTest, AllDirectionsHasSixEntries) {
    EXPECT_EQ(game::ALL_DIRECTIONS.size(), 6);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
```

- [ ] **Step 3: Add test to CMakeLists.txt**

Add `game/test_hex_direction.cpp` to the `add_executable(my4x_tests ...)` list in `tests/CMakeLists.txt`.

- [ ] **Step 4: Run tests, verify pass**

Run: `make ci`
Expected: All tests pass including new HexDirection tests.

- [ ] **Step 5: Commit**

```bash
git add game/include/game/HexDirection.h tests/game/test_hex_direction.cpp tests/CMakeLists.txt
git commit -m "feat: add HexDirection enum and reusable neighbor utilities"
```

---

## Task 2: Add integer elevation to Tile

**Linear:** TON-76 (prerequisite)

Tiles need an explicit integer elevation field (not just the terrain-based Y offset) so edges can be classified as flat/slope/cliff and terraces can be rendered.

**Files:**
- Modify: `game/include/game/Tile.h`
- Modify: `game/src/Tile.cpp`
- Modify: `game/src/Map.cpp`
- Modify: `tests/game/test_terrain.cpp`

- [ ] **Step 1: Add elevation field to Tile**

In `game/include/game/Tile.h`, add:
```cpp
[[nodiscard]] int elevation() const;
void setElevation(int elevation);
```
And private member `int elevation_{0};`.

In `game/src/Tile.cpp`, implement getter/setter.

- [ ] **Step 2: Set default elevations from terrain type**

In `game/src/Map.cpp`'s `generateTiles()`, after assigning terrain type, set default elevation:
```cpp
// After creating the tile:
int defaultElevation = defaultElevationForTerrain(terrain);
tile.setElevation(defaultElevation);
```

Add a local helper:
```cpp
static constexpr int WATER_ELEVATION = 0;
static constexpr int PLAINS_ELEVATION = 1;
static constexpr int DESERT_ELEVATION = 1;
static constexpr int SWAMP_ELEVATION = 1;
static constexpr int FOREST_ELEVATION = 1;
static constexpr int HILLS_ELEVATION = 2;
static constexpr int MOUNTAIN_ELEVATION = 4;

constexpr int defaultElevationForTerrain(TerrainType terrain) {
    switch (terrain) {
    case TerrainType::Water:    return WATER_ELEVATION;
    case TerrainType::Plains:   return PLAINS_ELEVATION;
    case TerrainType::Desert:   return DESERT_ELEVATION;
    case TerrainType::Swamp:    return SWAMP_ELEVATION;
    case TerrainType::Forest:   return FOREST_ELEVATION;
    case TerrainType::Hills:    return HILLS_ELEVATION;
    case TerrainType::Mountain: return MOUNTAIN_ELEVATION;
    }
    return PLAINS_ELEVATION;
}
```

- [ ] **Step 3: Add tests for elevation**

In `tests/game/test_terrain.cpp`, add tests:
```cpp
TEST(TileTest, DefaultElevation) {
    game::Tile plains(0, 0, game::TerrainType::Plains);
    // Direct construction defaults to 0
    EXPECT_EQ(plains.elevation(), 0);
}

TEST(TileTest, SetAndGetElevation) {
    game::Tile tile(0, 0, game::TerrainType::Plains);
    tile.setElevation(3);
    EXPECT_EQ(tile.elevation(), 3);
}

TEST(TileTest, MapGenerationSetsElevationFromTerrain) {
    game::Map map(5, 5, 42);
    for (int r = 0; r < map.height(); ++r) {
        for (int c = 0; c < map.width(); ++c) {
            const auto &t = map.tile(r, c);
            // Water should have elevation 0, mountains should have the highest
            if (t.terrainType() == game::TerrainType::Water) {
                EXPECT_EQ(t.elevation(), 0);
            } else if (t.terrainType() == game::TerrainType::Mountain) {
                EXPECT_EQ(t.elevation(), 4);
            }
            // All elevations should be non-negative
            EXPECT_GE(t.elevation(), 0);
        }
    }
}
```

- [ ] **Step 4: Run tests, verify pass**

Run: `make ci`

- [ ] **Step 5: Commit**

```bash
git add game/include/game/Tile.h game/src/Tile.cpp game/src/Map.cpp tests/game/test_terrain.cpp
git commit -m "feat: add integer elevation field to Tile with terrain-based defaults"
```

---

## Task 3: HexMetrics, HexEdgeType, and EdgeVertices

**Linear:** TON-75, TON-76, TON-77

Shared geometry constants and structures used by all subsequent rendering tasks.

**Files:**
- Create: `engine/include/engine/HexMetrics.h`
- Create: `engine/include/engine/HexEdgeType.h`
- Create: `engine/include/engine/EdgeVertices.h`

- [ ] **Step 1: Create HexMetrics.h**

```cpp
// engine/include/engine/HexMetrics.h
#pragma once

namespace engine::hex_metrics {

/// Hex radius (duplicated from HexGrid.cpp where it is file-scope).
/// This is the canonical constant for all hex geometry calculations.
static constexpr float HEX_RADIUS = 1.0F;

/// Fraction of the hex radius that remains solid-colored (no blending).
static constexpr float SOLID_FACTOR = 0.8F;

/// Fraction of the hex radius used for blend regions.
static constexpr float BLEND_FACTOR = 1.0F - SOLID_FACTOR;

/// World-space height per elevation level.
static constexpr float ELEVATION_STEP = 0.3F;

/// Number of terrace steps between two elevation levels.
static constexpr int TERRACE_STEPS = 2;

/// Total interpolation segments for terrace geometry (2 * steps + 1).
static constexpr int TERRACE_INTERPOLATION_STEPS = (TERRACE_STEPS * 2) + 1;

/// Horizontal interpolation increment per terrace step.
static constexpr float HORIZONTAL_TERRACE_STEP =
    1.0F / static_cast<float>(TERRACE_INTERPOLATION_STEPS);

/// Vertical interpolation increment per terrace step (steps in pairs).
static constexpr float VERTICAL_TERRACE_STEP =
    1.0F / static_cast<float>(TERRACE_STEPS + 1);

/// Perlin noise perturbation strength (XZ displacement in world units).
static constexpr float PERTURBATION_STRENGTH = 0.15F;

/// Perlin noise sampling scale (higher = more stretched, smoother noise).
static constexpr float NOISE_SCALE = 0.25F;

/// Elevation perturbation strength (Y displacement per cell).
static constexpr float ELEVATION_PERTURBATION_STRENGTH = 0.08F;

/// Number of edge subdivisions (creates this many + 1 vertices per edge).
static constexpr int EDGE_SUBDIVISIONS = 3;

/// Offset to sample a second independent noise axis (avoids correlation).
static constexpr float NOISE_AXIS_OFFSET = 100.0F;

/// Scale factor for elevation noise sampling (coarser than vertex noise).
static constexpr float ELEVATION_NOISE_SCALE = 0.5F;

} // namespace engine::hex_metrics
```

- [ ] **Step 2: Create HexEdgeType.h**

```cpp
// engine/include/engine/HexEdgeType.h
#pragma once

#include <cstdint>

namespace engine {

/// Classification of the edge between two hex cells based on elevation difference.
enum class HexEdgeType : std::uint8_t {
    Flat,  ///< Same elevation
    Slope, ///< Exactly 1 elevation level difference
    Cliff, ///< 2+ elevation levels difference
};

/// Classify the edge type between two elevations.
[[nodiscard]] inline constexpr HexEdgeType classifyEdge(int elevationA, int elevationB) {
    int diff = (elevationA > elevationB) ? (elevationA - elevationB) : (elevationB - elevationA);
    if (diff == 0) {
        return HexEdgeType::Flat;
    }
    if (diff == 1) {
        return HexEdgeType::Slope;
    }
    return HexEdgeType::Cliff;
}

} // namespace engine
```

- [ ] **Step 3: Create EdgeVertices.h**

```cpp
// engine/include/engine/EdgeVertices.h
#pragma once

#include "raylib.h"

namespace engine {

/// Four vertices defining a subdivided hex edge (3 segments).
/// v1 is the "left" corner, v4 is the "right" corner.
/// v2 and v3 are the intermediate subdivision points.
struct EdgeVertices {
    Vector3 v1;
    Vector3 v2;
    Vector3 v3;
    Vector3 v4;
};

/// Linearly interpolate between two Vector3 values.
[[nodiscard]] inline Vector3 lerpVector3(Vector3 a, Vector3 b, float t) {
    return {
        .x = a.x + (b.x - a.x) * t,
        .y = a.y + (b.y - a.y) * t,
        .z = a.z + (b.z - a.z) * t,
    };
}

/// Linearly interpolate between two Colors.
[[nodiscard]] inline Color lerpColor(Color a, Color b, float t) {
    return {
        .r = static_cast<unsigned char>(static_cast<float>(a.r) + (static_cast<float>(b.r) - static_cast<float>(a.r)) * t),
        .g = static_cast<unsigned char>(static_cast<float>(a.g) + (static_cast<float>(b.g) - static_cast<float>(a.g)) * t),
        .b = static_cast<unsigned char>(static_cast<float>(a.b) + (static_cast<float>(b.b) - static_cast<float>(a.b)) * t),
        .a = static_cast<unsigned char>(static_cast<float>(a.a) + (static_cast<float>(b.a) - static_cast<float>(a.a)) * t),
    };
}

/// Create an EdgeVertices by subdividing the line from corner1 to corner2 into 3 equal segments.
[[nodiscard]] inline EdgeVertices makeEdge(Vector3 corner1, Vector3 corner2) {
    static constexpr float ONE_THIRD = 1.0F / 3.0F;
    static constexpr float TWO_THIRDS = 2.0F / 3.0F;
    return {
        .v1 = corner1,
        .v2 = lerpVector3(corner1, corner2, ONE_THIRD),
        .v3 = lerpVector3(corner1, corner2, TWO_THIRDS),
        .v4 = corner2,
    };
}

} // namespace engine
```

- [ ] **Step 4: Verify build compiles**

Run: `make build`

- [ ] **Step 5: Commit**

```bash
git add engine/include/engine/HexMetrics.h engine/include/engine/HexEdgeType.h engine/include/engine/EdgeVertices.h
git commit -m "feat: add HexMetrics, HexEdgeType, and EdgeVertices shared utilities"
```

---

## Task 4: HexMeshBuilder for per-vertex-colored rendering

**Linear:** TON-75

raylib's `DrawTriangle3D` only supports a single color per triangle. For smooth terrain blending, we need per-vertex colors. `HexMeshBuilder` accumulates colored vertices and flushes them via rlgl.

**Files:**
- Create: `engine/include/engine/HexMeshBuilder.h`
- Create: `engine/src/HexMeshBuilder.cpp`
- Create: `tests/engine/test_hex_mesh_builder.cpp`
- Modify: `engine/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write HexMeshBuilder.h**

```cpp
// engine/include/engine/HexMeshBuilder.h
#pragma once

#include "raylib.h"

#include <vector>

namespace engine {

/// Accumulates colored triangles and renders them in one rlgl batch.
/// Usage: call addTriangle() repeatedly, then flush() to render.
class HexMeshBuilder {
  public:
    /// Add a triangle with per-vertex positions and colors.
    void addTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color c1, Color c2, Color c3);

    /// Add a triangle with a single uniform color (convenience).
    void addTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color color);

    /// Add a quad as two triangles (v1-v2-v3, v1-v3-v4) with per-vertex colors.
    void addQuad(Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4,
                 Color c1, Color c2, Color c3, Color c4);

    /// Render all accumulated triangles via rlgl and clear the buffers.
    void flush();

    /// Clear without rendering.
    void clear();

    /// Number of triangles currently buffered.
    [[nodiscard]] std::size_t triangleCount() const;

  private:
    struct Vertex {
        Vector3 position;
        Color color;
    };
    std::vector<Vertex> vertices_;
};

} // namespace engine
```

- [ ] **Step 2: Write HexMeshBuilder.cpp**

Note: `rlgl.h` is part of raylib's public headers and should be available via the existing `target_link_libraries(my4x_engine PUBLIC raylib)`. This is the first use of rlgl in the project — if the include doesn't resolve, try `#include <rlgl.h>` or check `build/_deps/raylib-src/src/rlgl.h`.

```cpp
// engine/src/HexMeshBuilder.cpp
#include "engine/HexMeshBuilder.h"

#include "rlgl.h"

namespace engine {

static constexpr int VERTICES_PER_TRIANGLE = 3;
static constexpr int VERTICES_PER_QUAD = 6; // 2 triangles

void HexMeshBuilder::addTriangle(Vector3 v1, Vector3 v2, Vector3 v3,
                                  Color c1, Color c2, Color c3) {
    vertices_.push_back({v1, c1});
    vertices_.push_back({v2, c2});
    vertices_.push_back({v3, c3});
}

void HexMeshBuilder::addTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color color) {
    addTriangle(v1, v2, v3, color, color, color);
}

void HexMeshBuilder::addQuad(Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4,
                              Color c1, Color c2, Color c3, Color c4) {
    // Triangle 1: v1, v2, v3
    addTriangle(v1, v2, v3, c1, c2, c3);
    // Triangle 2: v1, v3, v4
    addTriangle(v1, v3, v4, c1, c3, c4);
}

void HexMeshBuilder::flush() {
    if (vertices_.empty()) {
        return;
    }

    rlBegin(RL_TRIANGLES);
    for (const auto &vert : vertices_) {
        rlColor4ub(vert.color.r, vert.color.g, vert.color.b, vert.color.a);
        rlVertex3f(vert.position.x, vert.position.y, vert.position.z);
    }
    rlEnd();

    vertices_.clear();
}

void HexMeshBuilder::clear() {
    vertices_.clear();
}

std::size_t HexMeshBuilder::triangleCount() const {
    return vertices_.size() / VERTICES_PER_TRIANGLE;
}

} // namespace engine
```

- [ ] **Step 3: Write tests**

```cpp
// tests/engine/test_hex_mesh_builder.cpp
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/HexMeshBuilder.h"
#include <gtest/gtest.h>

TEST(HexMeshBuilderTest, StartsEmpty) {
    engine::HexMeshBuilder builder;
    EXPECT_EQ(builder.triangleCount(), 0);
}

TEST(HexMeshBuilderTest, AddTriangleIncreasesCount) {
    engine::HexMeshBuilder builder;
    Vector3 v1 = {0, 0, 0}, v2 = {1, 0, 0}, v3 = {0, 0, 1};
    Color c = {255, 0, 0, 255};
    builder.addTriangle(v1, v2, v3, c);
    EXPECT_EQ(builder.triangleCount(), 1);
}

TEST(HexMeshBuilderTest, AddQuadCreatesTwoTriangles) {
    engine::HexMeshBuilder builder;
    Vector3 v1 = {0, 0, 0}, v2 = {1, 0, 0}, v3 = {1, 0, 1}, v4 = {0, 0, 1};
    Color c = {0, 255, 0, 255};
    builder.addQuad(v1, v2, v3, v4, c, c, c, c);
    EXPECT_EQ(builder.triangleCount(), 2);
}

TEST(HexMeshBuilderTest, ClearRemovesAllTriangles) {
    engine::HexMeshBuilder builder;
    Vector3 v = {0, 0, 0};
    Color c = {0, 0, 0, 255};
    builder.addTriangle(v, v, v, c);
    builder.addTriangle(v, v, v, c);
    builder.clear();
    EXPECT_EQ(builder.triangleCount(), 0);
}

TEST(HexMeshBuilderTest, PerVertexColorTriangle) {
    engine::HexMeshBuilder builder;
    Vector3 v1 = {0, 0, 0}, v2 = {1, 0, 0}, v3 = {0, 0, 1};
    Color c1 = {255, 0, 0, 255}, c2 = {0, 255, 0, 255}, c3 = {0, 0, 255, 255};
    builder.addTriangle(v1, v2, v3, c1, c2, c3);
    EXPECT_EQ(builder.triangleCount(), 1);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
```

- [ ] **Step 4: Update CMakeLists.txt files**

In `engine/CMakeLists.txt`, add `src/HexMeshBuilder.cpp` to the source list.

In `tests/CMakeLists.txt`, add `engine/test_hex_mesh_builder.cpp` to the test list.

- [ ] **Step 5: Run tests, verify pass**

Run: `make ci`

- [ ] **Step 6: Commit**

```bash
git add engine/include/engine/HexMeshBuilder.h engine/src/HexMeshBuilder.cpp \
    tests/engine/test_hex_mesh_builder.cpp engine/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat: add HexMeshBuilder for per-vertex-colored triangle rendering"
```

---

## Task 5: Terrain color blending in MapRenderer

**Linear:** TON-75

Overhaul `MapRenderer::drawMap()` to triangulate hex cells with solid inner cores and blended edge/corner regions using `HexMeshBuilder` for per-vertex colors.

**Files:**
- Modify: `engine/src/MapRenderer.cpp`
- Modify: `engine/include/engine/MapRenderer.h` (add `game/HexDirection.h` include if needed)

- [ ] **Step 1: Add helper functions for hex geometry**

Add these static helpers to `MapRenderer.cpp`:

```cpp
#include "engine/HexMeshBuilder.h"
#include "engine/HexMetrics.h"
#include "engine/EdgeVertices.h"
#include "game/HexDirection.h"

// Compute the 6 corner vertices of a hex at given center and radius.
// Same math as hex::hexVertices() but parameterized by radius.
static std::array<Vector3, 6> hexCornersAtRadius(Vector3 center, float radius) {
    static constexpr float START_ANGLE = 30.0F;
    static constexpr float DEGREES_PER_SIDE = 60.0F;
    static constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0F;
    std::array<Vector3, 6> corners{};
    for (int i = 0; i < 6; ++i) {
        float angle = (START_ANGLE + static_cast<float>(i) * DEGREES_PER_SIDE) * DEG_TO_RAD;
        corners[i] = {
            .x = center.x + radius * cosf(angle),
            .y = center.y,
            .z = center.z + radius * sinf(angle),
        };
    }
    return corners;
}

// Get the color for a neighbor tile, or the cell's own color if no neighbor.
static Color getNeighborColor(const game::Map &map, int row, int col,
                               game::HexDirection dir, Color ownColor,
                               const game::FogOfWar *fog, game::FactionId playerId) {
    auto neighbor = game::neighborCoord(row, col, dir, map.height(), map.width());
    if (!neighbor) {
        return ownColor;
    }
    auto [nr, nc] = *neighbor;
    // If neighbor is unexplored, use own color (no blending with unseen terrain)
    if (fog && fog->getVisibility(playerId, nr, nc) == game::VisibilityState::Unexplored) {
        return ownColor;
    }
    return terrain_colors::terrainFillColor(map.tile(nr, nc).terrainType());
}
```

- [ ] **Step 2: Rewrite the tile rendering loop**

Replace the existing per-tile rendering with three-phase triangulation. The key structure:

```cpp
void drawMap(const game::Map &map, std::optional<hex::TileCoord> highlightedTile,
             const game::FogOfWar *fog, game::FactionId playerFactionId) {
    HexMeshBuilder builder;

    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            // --- Visibility check (keep existing fog logic) ---
            auto vis = fog ? fog->getVisibility(playerFactionId, row, col)
                           : game::VisibilityState::Visible;
            if (vis == game::VisibilityState::Unexplored) {
                // Draw solid black hex using old approach (drawFilledHex3D)
                continue;
            }

            Vector3 center = hex::tileCenter(row, col);
            const auto &tile = map.tile(row, col);
            center.y = terrain_height::terrainHeight(tile.terrainType());

            Color cellColor = terrain_colors::terrainFillColor(tile.terrainType());
            if (vis == game::VisibilityState::Explored) {
                cellColor = dimColor(cellColor);
            }

            auto innerCorners = hexCornersAtRadius(center, hex_metrics::HEX_RADIUS * hex_metrics::SOLID_FACTOR);
            auto outerCorners = hexCornersAtRadius(center, hex_metrics::HEX_RADIUS);

            // Phase 1: Solid inner hex (6 triangles, all cell color)
            for (int i = 0; i < 6; ++i) {
                int next = (i + 1) % 6;
                builder.addTriangle(center, innerCorners[next], innerCorners[i], cellColor);
            }

            // Phase 2: Edge blend quads (only first 3 directions to avoid double-draw)
            for (int d = 0; d < 3; ++d) {
                auto dir = game::ALL_DIRECTIONS[d];
                int i = d;           // vertex index for this direction
                int next = (i + 1) % 6;
                Color neighborColor = getNeighborColor(map, row, col, dir, cellColor, fog, playerFactionId);
                // Blend quad from inner edge to outer edge
                builder.addQuad(
                    innerCorners[i], innerCorners[next],    // near side (cell color)
                    outerCorners[next], outerCorners[i],    // far side (blended)
                    cellColor, cellColor, neighborColor, neighborColor
                );
            }

            // Phase 2b: Edge regions for remaining 3 directions (SW, W, NW)
            // These are the "back" edges that weren't covered by a neighbor's Phase 2.
            // Only draw if the neighbor doesn't exist (map edge).
            for (int d = 3; d < 6; ++d) {
                auto dir = game::ALL_DIRECTIONS[d];
                auto neighbor = game::neighborCoord(row, col, dir, map.height(), map.width());
                if (!neighbor) {
                    int i = d;
                    int next = (i + 1) % 6;
                    builder.addQuad(
                        innerCorners[i], innerCorners[next],
                        outerCorners[next], outerCorners[i],
                        cellColor, cellColor, cellColor, cellColor
                    );
                }
            }

            // Phase 3: Corner triangles (where 3 cells meet)
            // Each corner vertex is shared by the current cell and two neighbors.
            // Only process corners on the "first 3" side to avoid triple-draw.
            for (int d = 0; d < 3; ++d) {
                int i = (d + 1) % 6;  // corner vertex between direction d and d+1
                auto dir1 = game::ALL_DIRECTIONS[d];
                auto dir2 = game::ALL_DIRECTIONS[(d + 1) % 6];
                Color c1 = getNeighborColor(map, row, col, dir1, cellColor, fog, playerFactionId);
                Color c2 = getNeighborColor(map, row, col, dir2, cellColor, fog, playerFactionId);
                builder.addTriangle(
                    innerCorners[i], outerCorners[i],
                    lerpVector3(outerCorners[i], center, hex_metrics::BLEND_FACTOR),
                    cellColor, c1, c2
                );
            }
        }
    }

    builder.flush();

    // --- Decorations and highlights (keep existing code) ---
    // Draw these AFTER the blended terrain mesh, using the old DrawTriangle3D/DrawCylinderEx calls.
    // ... (existing drawForestTrees, drawMountainPeak, drawHillsBump, highlight logic)
}
```

**Important notes for the implementer:**
- The vertex index mapping between `HexDirection` and corner vertices depends on the hex orientation (flat-top, starting at 30 degrees). You'll need to verify which corner vertex aligns with which direction and adjust the index mapping. The existing `hexVertices()` starts at 30 degrees going counter-clockwise.
- The corner triangle geometry above is simplified. In practice, corner vertices where 3 cells meet need careful positioning. Start with the basic version and refine visually.
- Keep the existing decoration drawing functions (`drawForestTrees`, `drawMountainPeak`, `drawHillsBump`) as-is — they render on top of the blended terrain.
- Keep the existing fog overlay for unexplored tiles using `drawFilledHex3D`.

- [ ] **Step 3: Verify visually**

Run the game: `make build && ./build/app/my4x`
Expected: Terrain colors blend smoothly between adjacent cells. Hex identity remains clear via solid core. Fog of war still works. Decorations render on top.

- [ ] **Step 4: Run CI**

Run: `make ci`

- [ ] **Step 5: Commit**

```bash
git add engine/src/MapRenderer.cpp engine/include/engine/MapRenderer.h
git commit -m "feat(TON-75): implement terrain color blending between adjacent cells"
```

---

## Task 6: Elevation-aware rendering and terrace geometry

**Linear:** TON-76

Use the integer elevation from Task 2 to position cells at different Y levels. Classify edges as flat/slope/cliff and render slopes as stepped terraces.

**Files:**
- Modify: `engine/src/MapRenderer.cpp`

- [ ] **Step 1: Apply elevation-based Y positioning**

Replace the terrain-type-based height offset (`terrain_height::terrainHeight()`) with elevation-based positioning:
```cpp
float cellY = static_cast<float>(tile.elevation()) * hex_metrics::ELEVATION_STEP;
center.y = cellY;
```

- [ ] **Step 2: Implement terrace interpolation for slope edges**

When an edge is classified as `HexEdgeType::Slope`, replace the simple blend quad with terraced steps.

Add a `terraceInterpolate` helper:
```cpp
/// Interpolate position between two points with terrace stepping.
/// step ranges from 1 to TERRACE_INTERPOLATION_STEPS.
/// XZ interpolates smoothly, Y snaps to discrete steps.
static Vector3 terraceInterpolate(Vector3 a, Vector3 b, int step) {
    float h = static_cast<float>(step) * hex_metrics::HORIZONTAL_TERRACE_STEP;
    Vector3 result = {
        .x = a.x + (b.x - a.x) * h,
        .y = a.y,  // will be adjusted below
        .z = a.z + (b.z - a.z) * h,
    };
    // Y steps in discrete increments (odd steps = flat, even steps = rise)
    float v = static_cast<float>((step + 1) / 2) * hex_metrics::VERTICAL_TERRACE_STEP;
    result.y = a.y + (b.y - a.y) * v;
    return result;
}

/// Same for color interpolation along terraces.
static Color terraceColorInterpolate(Color a, Color b, int step) {
    float h = static_cast<float>(step) * hex_metrics::HORIZONTAL_TERRACE_STEP;
    return lerpColor(a, b, h);
}
```

For slope edges, replace the single blend quad with a series of quads — one per terrace step:
```cpp
if (edgeType == HexEdgeType::Slope) {
    // Starting vertices: inner edge corners of current cell
    Vector3 prevLeft = innerCorners[i];
    Vector3 prevRight = innerCorners[next];
    Color prevColor = cellColor;
    for (int step = 1; step <= hex_metrics::TERRACE_INTERPOLATION_STEPS; ++step) {
        Vector3 curLeft = terraceInterpolate(innerCorners[i], neighborInnerCorners[neighborI], step);
        Vector3 curRight = terraceInterpolate(innerCorners[next], neighborInnerCorners[neighborNext], step);
        Color curColor = terraceColorInterpolate(cellColor, neighborColor, step);
        builder.addQuad(prevLeft, prevRight, curRight, curLeft, prevColor, prevColor, curColor, curColor);
        prevLeft = curLeft;
        prevRight = curRight;
        prevColor = curColor;
    }
}
```
Note: `neighborInnerCorners` must be computed for the neighbor cell (same `hexCornersAtRadius` call at the neighbor's center and elevation). The `neighborI`/`neighborNext` indices are the opposite-direction corner indices on the neighbor cell.

For flat edges: render as simple blend quads (no terracing needed).
For cliff edges: render as vertical faces or leave gaps (implementation choice — vertical cliff faces are a good visual).

- [ ] **Step 3: Handle corner terracing**

Corners where 3 cells meet have 8 possible edge-type configurations. The key cases:
- All flat: simple triangle
- Slope-slope-flat: terraced triangle
- Slope-cliff: boundary interpolation (collapse terraces toward cliff boundary point)
- All cliff: omit or render as vertical wall

For initial implementation, handle slope-slope-flat and leave cliff corners as vertical faces. More complex configurations can be refined later.

- [ ] **Step 4: Verify visually**

Run the game. Expected: Hills and mountains sit above plains/water. Slopes between elevation levels show stepped terraces. Cliffs are visible as vertical faces.

- [ ] **Step 5: Run CI**

Run: `make ci`

- [ ] **Step 6: Commit**

```bash
git add engine/src/MapRenderer.cpp
git commit -m "feat(TON-76): implement elevation-aware rendering with terraced slopes"
```

---

## Task 7: Edge subdivision

**Linear:** TON-77

Subdivide each hex edge into 3 segments using `EdgeVertices`, creating smoother geometry for noise perturbation and future river channel support.

**Files:**
- Modify: `engine/src/MapRenderer.cpp`

- [ ] **Step 1: Replace simple edge vertices with EdgeVertices**

Update the inner hex triangulation to use `EdgeVertices`:
- For each of the 6 hex edges, compute `EdgeVertices edge = makeEdge(innerCorner1, innerCorner2)`
- Triangulate the inner hex as a fan: center → edge.v1, center → edge.v2, center → edge.v3, center → edge.v4 (3 triangles per edge instead of 1, for a total of 18).

- [ ] **Step 2: Update edge connection quads to use EdgeVertices**

Edge blend quads now use `EdgeVertices` on both sides:
- `EdgeVertices nearEdge = makeEdge(innerCornerA, innerCornerB);`
- `EdgeVertices farEdge = makeEdge(outerCornerA, outerCornerB);`
- Draw 3 quads connecting nearEdge.v1→v2 to farEdge.v1→v2, etc.

- [ ] **Step 3: Update terrace interpolation with EdgeVertices**

Terraced slopes now interpolate between `EdgeVertices` pairs instead of single vertex pairs. Each terrace step creates a strip of 3 quads.

- [ ] **Step 4: Verify visually**

Run the game. Expected: Hexes look identical to before (more triangles, same positions). The subdivision is invisible until noise is applied.

- [ ] **Step 5: Run CI**

Run: `make ci`

- [ ] **Step 6: Commit**

```bash
git add engine/src/MapRenderer.cpp
git commit -m "feat(TON-77): subdivide hex edges into 3 segments via EdgeVertices"
```

---

## Task 8: Perlin noise perturbation

**Linear:** TON-78

Apply Perlin noise to hex vertices to break up the regular grid into organic, natural-looking terrain.

**Files:**
- Create: `engine/include/engine/PerlinNoise.h`
- Create: `engine/src/PerlinNoise.cpp`
- Create: `tests/engine/test_perlin_noise.cpp`
- Modify: `engine/src/MapRenderer.cpp`
- Modify: `engine/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Implement PerlinNoise**

```cpp
// engine/include/engine/PerlinNoise.h
#pragma once

#include <cstdint>

namespace engine {

/// Deterministic 2D Perlin noise generator.
/// Returns values in approximately [-1, 1] range.
class PerlinNoise {
  public:
    /// Construct with a seed for reproducible noise.
    explicit PerlinNoise(std::uint64_t seed = 0);

    /// Sample 2D noise at (x, y). Returns value in ~[-1, 1].
    [[nodiscard]] float sample(float x, float y) const;

  private:
    static constexpr int TABLE_SIZE = 256;
    int perm_[TABLE_SIZE * 2]{};

    [[nodiscard]] float grad(int hash, float x, float y) const;
    [[nodiscard]] static float fade(float t);
};

} // namespace engine
```

Implementation in `PerlinNoise.cpp`: standard 2D Perlin noise with permutation table shuffled by seed.

- [ ] **Step 2: Write Perlin noise tests**

```cpp
// tests/engine/test_perlin_noise.cpp
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/PerlinNoise.h"
#include <gtest/gtest.h>
#include <cmath>

TEST(PerlinNoiseTest, DeterministicSameSeed) {
    engine::PerlinNoise a(42);
    engine::PerlinNoise b(42);
    EXPECT_FLOAT_EQ(a.sample(1.5F, 2.3F), b.sample(1.5F, 2.3F));
    EXPECT_FLOAT_EQ(a.sample(0.0F, 0.0F), b.sample(0.0F, 0.0F));
}

TEST(PerlinNoiseTest, DifferentSeedsDifferentValues) {
    engine::PerlinNoise a(1);
    engine::PerlinNoise b(2);
    // Very unlikely to be exactly equal
    EXPECT_NE(a.sample(3.7F, 8.1F), b.sample(3.7F, 8.1F));
}

TEST(PerlinNoiseTest, OutputRange) {
    engine::PerlinNoise noise(123);
    for (float x = -10.0F; x < 10.0F; x += 0.37F) {
        for (float y = -10.0F; y < 10.0F; y += 0.37F) {
            float val = noise.sample(x, y);
            EXPECT_GE(val, -1.5F);
            EXPECT_LE(val, 1.5F);
        }
    }
}

TEST(PerlinNoiseTest, SmoothContinuity) {
    engine::PerlinNoise noise(99);
    float v1 = noise.sample(5.0F, 5.0F);
    float v2 = noise.sample(5.001F, 5.0F);
    // Nearby samples should be close
    EXPECT_NEAR(v1, v2, 0.1F);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
```

- [ ] **Step 3: Update CMakeLists.txt files**

Add `src/PerlinNoise.cpp` to `engine/CMakeLists.txt`.
Add `engine/test_perlin_noise.cpp` to `tests/CMakeLists.txt`.

- [ ] **Step 4: Run tests**

Run: `make ci`

- [ ] **Step 5: Commit noise module**

```bash
git add engine/include/engine/PerlinNoise.h engine/src/PerlinNoise.cpp \
    tests/engine/test_perlin_noise.cpp engine/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat: add deterministic 2D Perlin noise generator"
```

- [ ] **Step 6: Apply perturbation in MapRenderer**

In `MapRenderer.cpp`, create a static `PerlinNoise` instance (seeded from map seed or a constant).

Add a `perturb()` helper:
```cpp
static Vector3 perturb(Vector3 position, const PerlinNoise &noise) {
    float sampleX = position.x * hex_metrics::NOISE_SCALE;
    float sampleZ = position.z * hex_metrics::NOISE_SCALE;
    return {
        .x = position.x + noise.sample(sampleX, sampleZ) * hex_metrics::PERTURBATION_STRENGTH,
        .y = position.y,  // Y unchanged by vertex perturbation
        .z = position.z + noise.sample(sampleX + hex_metrics::NOISE_AXIS_OFFSET,
                                        sampleZ + hex_metrics::NOISE_AXIS_OFFSET) * hex_metrics::PERTURBATION_STRENGTH,
    };
}
```

Apply `perturb()` to all outer and intermediate vertices. Do NOT perturb:
- Cell centers (keeps labels/units well-positioned)
- Cliff boundary vertices (prevents mesh cracks)

- [ ] **Step 7: Add per-cell elevation jitter**

Apply small Y offset per cell based on noise sampled at the cell center:
```cpp
float elevJitter = noise.sample(center.x * hex_metrics::ELEVATION_NOISE_SCALE,
                                 center.z * hex_metrics::ELEVATION_NOISE_SCALE)
                   * hex_metrics::ELEVATION_PERTURBATION_STRENGTH;
center.y += elevJitter;
```

This makes elevation look less perfectly stepped.

- [ ] **Step 8: Verify visually**

Run the game. Expected: Hex edges are no longer perfectly straight — they wiggle organically. Cell surfaces remain flat enough for units/labels. No visible mesh cracks.

- [ ] **Step 9: Run CI**

Run: `make ci`

- [ ] **Step 10: Commit**

```bash
git add engine/src/MapRenderer.cpp
git commit -m "feat(TON-78): apply Perlin noise perturbation for natural terrain irregularity"
```

---

## Task 9: Integration verification and cleanup

**Linear:** All TON-75 through TON-78

Final pass to ensure everything works together and existing systems aren't broken.

- [ ] **Step 1: Verify fog of war still works**

Unexplored tiles should still render as solid black. Explored tiles should be dimmed. The blending, terracing, and noise should respect visibility states.

- [ ] **Step 2: Verify unit/city/building rendering**

Units, cities, and buildings should render on top of the new terrain geometry without z-fighting or positioning issues. Decorations (trees, mountains, hills) should still appear correctly.

- [ ] **Step 3: Verify minimap**

The minimap should still reflect terrain colors correctly.

- [ ] **Step 4: Verify selection/highlighting**

Tile selection via mouse click should still work. The yellow highlight should render on top of blended terrain.

- [ ] **Step 5: Full CI pass**

Run: `make ci`

- [ ] **Step 6: Final commit if any fixups needed**

```bash
git add <specific-changed-files>
git commit -m "fix: integration adjustments for terrain foundation milestone"
```

---

## Known Gaps (Out of Scope)

- **Serialization:** The new `elevation_` field on `Tile` is not yet serialized via protobuf (`Serialization.cpp`, `SaveLoad.cpp`, `tile.proto`). Existing saves will default to elevation 0. This should be addressed in a follow-up task when save/load is next touched.
- **Pathfinding neighbor refactor:** `Pathfinding.cpp` still has its own local `EVEN_ROW_DIRECTIONS`/`ODD_ROW_DIRECTIONS` arrays. These should be migrated to use `HexDirection.h` in a future cleanup, but changing pathfinding is out of scope for this visual milestone.
