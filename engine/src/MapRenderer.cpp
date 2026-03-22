#include "engine/MapRenderer.h"

#include "engine/EdgeVertices.h"
#include "engine/HexEdgeType.h"
#include "engine/HexGrid.h"
#include "engine/HexMeshBuilder.h"
#include "engine/HexMetrics.h"
#include "engine/PerlinNoise.h"
#include "engine/TerrainColors.h"
#include "game/HexDirection.h"

#include "raylib.h"

#include <array>
#include <cmath>
#include <numbers>

namespace engine {

// ── Constants ────────────────────────────────────────────────────────────────

static constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0F;
static constexpr int HEX_SIDES = 6;
static constexpr int NEAR_DIRECTION_COUNT = 3;
static constexpr uint64_t NOISE_SEED = 42;

/// Global Perlin noise generator for vertex perturbation.
static const PerlinNoise &noiseGenerator() {
    static const PerlinNoise noise(NOISE_SEED);
    return noise;
}

/// Maps HexDirection index to the two corner vertex indices forming that edge.
static constexpr std::array<std::array<int, 2>, HEX_SIDES> DIRECTION_EDGE = {{
    {4, 5}, // NE
    {5, 0}, // E
    {0, 1}, // SE
    {1, 2}, // SW
    {2, 3}, // W
    {3, 4}, // NW
}};

/// Corner offset vectors from hex center at FULL radius (HEX_RADIUS = 1.0).
/// Cached on first call.
static const std::array<Vector3, HEX_SIDES> &cornerOffsets() {
    static constexpr float START = 30.0F;
    static constexpr float STEP = 60.0F;
    static const auto offsets = []() {
        std::array<Vector3, HEX_SIDES> o{};
        for (int i = 0; i < HEX_SIDES; ++i) {
            float angle = (START + (static_cast<float>(i) * STEP)) * DEG_TO_RAD;
            o.at(i) = {
                .x = hex_metrics::HEX_RADIUS * cosf(angle), .y = 0.0F, .z = hex_metrics::HEX_RADIUS * sinf(angle)};
        }
        return o;
    }();
    return offsets;
}

/// Get first solid corner for direction d: corners[DIRECTION_EDGE[d][0]] * solidFactor
static Vector3 getFirstSolidCorner(int dirIdx) {
    const auto &c = cornerOffsets();
    int idx = DIRECTION_EDGE.at(dirIdx).at(0);
    return {.x = c.at(idx).x * hex_metrics::SOLID_FACTOR, .y = 0.0F, .z = c.at(idx).z * hex_metrics::SOLID_FACTOR};
}

/// Get second solid corner for direction d: corners[DIRECTION_EDGE[d][1]] * solidFactor
static Vector3 getSecondSolidCorner(int dirIdx) {
    const auto &c = cornerOffsets();
    int idx = DIRECTION_EDGE.at(dirIdx).at(1);
    return {.x = c.at(idx).x * hex_metrics::SOLID_FACTOR, .y = 0.0F, .z = c.at(idx).z * hex_metrics::SOLID_FACTOR};
}

/// Bridge vector for direction d: (corners[left] + corners[right]) * blendFactor
static Vector3 getBridge(int dirIdx) {
    const auto &c = cornerOffsets();
    int left = DIRECTION_EDGE.at(dirIdx).at(0);
    int right = DIRECTION_EDGE.at(dirIdx).at(1);
    return {
        .x = (c.at(left).x + c.at(right).x) * hex_metrics::BLEND_FACTOR,
        .y = 0.0F,
        .z = (c.at(left).z + c.at(right).z) * hex_metrics::BLEND_FACTOR,
    };
}

static float elevationY(const game::Map &map, int r, int c) {
    return static_cast<float>(map.tile(r, c).elevation()) * hex_metrics::ELEVATION_STEP;
}

static Color getCellColor(const game::Map &map, int row, int col) {
    return terrain_colors::terrainFillColor(map.tile(row, col).terrainType());
}

// ── Perlin noise perturbation ────────────────────────────────────────────────

/// Perturb a vertex position using Perlin noise (XZ only, Y unchanged).
static Vector3 perturb(Vector3 pos) {
    const auto &noise = noiseGenerator();
    float sx = pos.x * hex_metrics::NOISE_SCALE;
    float sz = pos.z * hex_metrics::NOISE_SCALE;
    return {
        .x = pos.x + (noise.sample(sx, sz) * hex_metrics::PERTURBATION_STRENGTH),
        .y = pos.y,
        .z = pos.z + (noise.sample(sx + hex_metrics::NOISE_AXIS_OFFSET, sz + hex_metrics::NOISE_AXIS_OFFSET) *
                      hex_metrics::PERTURBATION_STRENGTH),
    };
}

// ── Edge strip helpers ───────────────────────────────────────────────────────

/// Triangulate a strip of triangles from center to an EdgeVertices (triangle fan).
/// Perturbs edge vertices but NOT the center.
static void triangulateEdgeFan(HexMeshBuilder &builder, Vector3 center, EdgeVertices edge, Color color) {
    builder.addTriangle(center, perturb(edge.v1), perturb(edge.v2), color);
    builder.addTriangle(center, perturb(edge.v2), perturb(edge.v3), color);
    builder.addTriangle(center, perturb(edge.v3), perturb(edge.v4), color);
}

/// Triangulate a quad strip between two EdgeVertices (3 quads).
/// Perturbs all vertices.
static void triangulateEdgeStrip(HexMeshBuilder &builder, EdgeVertices e1, Color c1, EdgeVertices e2, Color c2) {
    builder.addQuad(perturb(e1.v1), perturb(e1.v2), perturb(e2.v2), perturb(e2.v1), c1, c1, c2, c2);
    builder.addQuad(perturb(e1.v2), perturb(e1.v3), perturb(e2.v3), perturb(e2.v2), c1, c1, c2, c2);
    builder.addQuad(perturb(e1.v3), perturb(e1.v4), perturb(e2.v4), perturb(e2.v3), c1, c1, c2, c2);
}

// ── Terrace interpolation (Part 3) ───────────────────────────────────────────

static Vector3 terraceLerp(Vector3 a, Vector3 b, int step) {
    float h = static_cast<float>(step) * hex_metrics::HORIZONTAL_TERRACE_STEP;
    Vector3 result = {
        .x = a.x + ((b.x - a.x) * h),
        .y = a.y, // set below
        .z = a.z + ((b.z - a.z) * h),
    };
    int vStep = (step + 1) / 2; // NOLINT(bugprone-integer-division)
    float v = static_cast<float>(vStep) * hex_metrics::VERTICAL_TERRACE_STEP;
    result.y = a.y + ((b.y - a.y) * v);
    return result;
}

static Color terraceLerpColor(Color a, Color b, int step) {
    float h = static_cast<float>(step) * hex_metrics::HORIZONTAL_TERRACE_STEP;
    return lerpColor(a, b, h);
}

/// Interpolate an EdgeVertices between two edge positions at a terrace step.
static EdgeVertices terraceEdgeLerp(EdgeVertices a, EdgeVertices b, int step) {
    return {
        .v1 = terraceLerp(a.v1, b.v1, step),
        .v2 = terraceLerp(a.v2, b.v2, step),
        .v3 = terraceLerp(a.v3, b.v3, step),
        .v4 = terraceLerp(a.v4, b.v4, step),
    };
}

/// Edge terraces between two cells (slope connection) using EdgeVertices.
static void triangulateEdgeTerraces(HexMeshBuilder &builder, EdgeVertices begin, Color beginColor, EdgeVertices end,
                                    Color endColor) {
    EdgeVertices e2 = terraceEdgeLerp(begin, end, 1);
    Color c2 = terraceLerpColor(beginColor, endColor, 1);

    triangulateEdgeStrip(builder, begin, beginColor, e2, c2);

    for (int i = 2; i < hex_metrics::TERRACE_INTERPOLATION_STEPS; ++i) {
        EdgeVertices e1 = e2;
        Color c1 = c2;
        e2 = terraceEdgeLerp(begin, end, i);
        c2 = terraceLerpColor(beginColor, endColor, i);
        triangulateEdgeStrip(builder, e1, c1, e2, c2);
    }

    triangulateEdgeStrip(builder, e2, c2, end, endColor);
}

// ── Corner triangulation (Part 3 — all cases) ───────────────────────────────
// Corner vertices are perturbed except cliff boundary vertices.
// NOLINTBEGIN(readability-suspicious-call-argument)

static void triangulateCornerTerraces(HexMeshBuilder &builder, Vector3 begin, Color beginColor, Vector3 left,
                                      Color leftColor, Vector3 right, Color rightColor) {
    Vector3 v3 = terraceLerp(begin, left, 1);
    Vector3 v4 = terraceLerp(begin, right, 1);
    Color c3 = terraceLerpColor(beginColor, leftColor, 1);
    Color c4 = terraceLerpColor(beginColor, rightColor, 1);

    builder.addTriangle(perturb(begin), perturb(v3), perturb(v4), beginColor, c3, c4);

    for (int i = 2; i < hex_metrics::TERRACE_INTERPOLATION_STEPS; ++i) {
        Vector3 v1 = v3;
        Vector3 v2 = v4;
        Color c1 = c3;
        Color c2 = c4;
        v3 = terraceLerp(begin, left, i);
        v4 = terraceLerp(begin, right, i);
        c3 = terraceLerpColor(beginColor, leftColor, i);
        c4 = terraceLerpColor(beginColor, rightColor, i);
        builder.addQuad(perturb(v1), perturb(v2), perturb(v4), perturb(v3), c1, c2, c4, c3);
    }

    builder.addQuad(perturb(v3), perturb(v4), perturb(right), perturb(left), c3, c4, rightColor, leftColor);
}

static void triangulateBoundaryTriangle(HexMeshBuilder &builder, Vector3 begin, Color beginColor, Vector3 left,
                                        Color leftColor, Vector3 boundary, Color boundaryColor) {
    // Boundary vertex is NOT perturbed (prevents mesh cracks at cliff boundaries).
    Vector3 v2 = terraceLerp(begin, left, 1);
    Color c2 = terraceLerpColor(beginColor, leftColor, 1);

    builder.addTriangle(perturb(begin), perturb(v2), boundary, beginColor, c2, boundaryColor);

    for (int i = 2; i < hex_metrics::TERRACE_INTERPOLATION_STEPS; ++i) {
        Vector3 v1 = v2;
        Color c1 = c2;
        v2 = terraceLerp(begin, left, i);
        c2 = terraceLerpColor(beginColor, leftColor, i);
        builder.addTriangle(perturb(v1), perturb(v2), boundary, c1, c2, boundaryColor);
    }

    builder.addTriangle(perturb(v2), perturb(left), boundary, c2, leftColor, boundaryColor);
}

static void triangulateCornerTerracesCliff(HexMeshBuilder &builder, Vector3 begin, Color beginColor, int beginElev,
                                           Vector3 left, Color leftColor, int leftElev, Vector3 right, Color rightColor,
                                           int rightElev) {
    float b = 1.0F / static_cast<float>(rightElev - beginElev);
    if (b < 0.0F) {
        b = -b;
    }
    Vector3 boundary = lerpVector3(begin, right, b);
    Color boundaryColor = lerpColor(beginColor, rightColor, b);

    triangulateBoundaryTriangle(builder, begin, beginColor, left, leftColor, boundary, boundaryColor);

    if (classifyEdge(leftElev, rightElev) == HexEdgeType::Slope) {
        triangulateBoundaryTriangle(builder, left, leftColor, right, rightColor, boundary, boundaryColor);
    } else {
        builder.addTriangle(perturb(left), perturb(right), boundary, leftColor, rightColor, boundaryColor);
    }
}

static void triangulateCornerCliffTerraces(HexMeshBuilder &builder, Vector3 begin, Color beginColor, int beginElev,
                                           Vector3 left, Color leftColor, int leftElev, Vector3 right, Color rightColor,
                                           int rightElev) {
    float b = 1.0F / static_cast<float>(leftElev - beginElev);
    if (b < 0.0F) {
        b = -b;
    }
    Vector3 boundary = lerpVector3(begin, left, b);
    Color boundaryColor = lerpColor(beginColor, leftColor, b);

    triangulateBoundaryTriangle(builder, right, rightColor, begin, beginColor, boundary, boundaryColor);

    if (classifyEdge(leftElev, rightElev) == HexEdgeType::Slope) {
        triangulateBoundaryTriangle(builder, left, leftColor, right, rightColor, boundary, boundaryColor);
    } else {
        builder.addTriangle(perturb(left), perturb(right), boundary, leftColor, rightColor, boundaryColor);
    }
}

static void triangulateCorner(HexMeshBuilder &builder, Vector3 bottom, Color bottomColor, int bottomElev, Vector3 left,
                              Color leftColor, int leftElev, Vector3 right, Color rightColor, int rightElev) {
    auto leftEdgeType = classifyEdge(bottomElev, leftElev);
    auto rightEdgeType = classifyEdge(bottomElev, rightElev);

    if (leftEdgeType == HexEdgeType::Slope) {
        if (rightEdgeType == HexEdgeType::Slope) {
            triangulateCornerTerraces(builder, bottom, bottomColor, left, leftColor, right, rightColor);
        } else if (rightEdgeType == HexEdgeType::Flat) {
            triangulateCornerTerraces(builder, left, leftColor, right, rightColor, bottom, bottomColor);
        } else {
            triangulateCornerTerracesCliff(builder, bottom, bottomColor, bottomElev, left, leftColor, leftElev, right,
                                           rightColor, rightElev);
        }
    } else if (rightEdgeType == HexEdgeType::Slope) {
        if (leftEdgeType == HexEdgeType::Flat) {
            triangulateCornerTerraces(builder, right, rightColor, bottom, bottomColor, left, leftColor);
        } else {
            triangulateCornerCliffTerraces(builder, bottom, bottomColor, bottomElev, left, leftColor, leftElev, right,
                                           rightColor, rightElev);
        }
    } else if (classifyEdge(leftElev, rightElev) == HexEdgeType::Slope) {
        if (leftElev < rightElev) {
            triangulateCornerCliffTerraces(builder, right, rightColor, rightElev, bottom, bottomColor, bottomElev, left,
                                           leftColor, leftElev);
        } else {
            triangulateCornerTerracesCliff(builder, left, leftColor, leftElev, right, rightColor, rightElev, bottom,
                                           bottomColor, bottomElev);
        }
    } else {
        builder.addTriangle(perturb(bottom), perturb(left), perturb(right), bottomColor, leftColor, rightColor);
    }
}

// NOLINTEND(readability-suspicious-call-argument)

// ── Connection logic (Part 2 + Part 3 + Part 4 edge subdivision) ─────────────

/// Build the bridge quad strip and corner triangle for one direction.
/// Called only for NE, E, SE (d=0,1,2).
// NOLINTBEGIN(readability-suspicious-call-argument)
static void triangulateConnection(HexMeshBuilder &builder, const game::Map &map, int row, int col, int dirIdx,
                                  EdgeVertices e1, Color cellColor, int cellElev) {
    auto dir = game::ALL_DIRECTIONS.at(dirIdx);
    auto neighborOpt = game::neighborCoord(row, col, dir, map.height(), map.width());
    if (!neighborOpt) {
        return;
    }
    auto [nr, nc] = *neighborOpt;
    Color neighborColor = getCellColor(map, nr, nc);
    int neighborElev = map.tile(nr, nc).elevation();

    // Bridge vector and neighbor edge vertices.
    Vector3 bridge = getBridge(dirIdx);
    float neighborY = elevationY(map, nr, nc);
    EdgeVertices e2 = {
        .v1 = {.x = e1.v1.x + bridge.x, .y = neighborY, .z = e1.v1.z + bridge.z},
        .v2 = {.x = e1.v2.x + bridge.x, .y = neighborY, .z = e1.v2.z + bridge.z},
        .v3 = {.x = e1.v3.x + bridge.x, .y = neighborY, .z = e1.v3.z + bridge.z},
        .v4 = {.x = e1.v4.x + bridge.x, .y = neighborY, .z = e1.v4.z + bridge.z},
    };

    auto edgeType = classifyEdge(cellElev, neighborElev);
    if (edgeType == HexEdgeType::Slope) {
        triangulateEdgeTerraces(builder, e1, cellColor, e2, neighborColor);
    } else {
        triangulateEdgeStrip(builder, e1, cellColor, e2, neighborColor);
    }

    // Corner triangle: only for NE (d=0) and E (d=1).
    int nextD = (dirIdx + 1) % HEX_SIDES;
    auto nextDir = game::ALL_DIRECTIONS.at(nextD);
    auto nextNeighborOpt = game::neighborCoord(row, col, nextDir, map.height(), map.width());
    if (dirIdx <= 1 && nextNeighborOpt) {
        auto [nnr, nnc] = *nextNeighborOpt;
        Color nextNeighborColor = getCellColor(map, nnr, nnc);
        int nextNeighborElev = map.tile(nnr, nnc).elevation();

        // v5 = e1.v4 + bridge(nextDirection), with next neighbor's Y.
        Vector3 nextBridge = getBridge(nextD);
        Vector3 v5 = {.x = e1.v4.x + nextBridge.x, .y = elevationY(map, nnr, nnc), .z = e1.v4.z + nextBridge.z};

        // Determine bottom cell and dispatch.
        if (cellElev <= neighborElev) {
            if (cellElev <= nextNeighborElev) {
                triangulateCorner(builder, e1.v4, cellColor, cellElev, e2.v4, neighborColor, neighborElev, v5,
                                  nextNeighborColor, nextNeighborElev);
            } else {
                triangulateCorner(builder, v5, nextNeighborColor, nextNeighborElev, e1.v4, cellColor, cellElev, e2.v4,
                                  neighborColor, neighborElev);
            }
        } else if (neighborElev <= nextNeighborElev) {
            triangulateCorner(builder, e2.v4, neighborColor, neighborElev, v5, nextNeighborColor, nextNeighborElev,
                              e1.v4, cellColor, cellElev);
        } else {
            triangulateCorner(builder, v5, nextNeighborColor, nextNeighborElev, e1.v4, cellColor, cellElev, e2.v4,
                              neighborColor, neighborElev);
        }
    }
}

// NOLINTEND(readability-suspicious-call-argument)

// ── Per-cell triangulation ───────────────────────────────────────────────────

static void triangulateCell(HexMeshBuilder &builder, const game::Map &map, int row, int col) {
    Vector3 center = hex::tileCenter(row, col);
    center.y = elevationY(map, row, col);

    // Add per-cell elevation jitter from noise for natural Y variation.
    const auto &noise = noiseGenerator();
    float elevJitter =
        noise.sample(center.x * hex_metrics::ELEVATION_NOISE_SCALE, center.z * hex_metrics::ELEVATION_NOISE_SCALE) *
        hex_metrics::ELEVATION_PERTURBATION_STRENGTH;
    center.y += elevJitter;

    Color cellColor = getCellColor(map, row, col);
    int cellElev = map.tile(row, col).elevation();

    for (int d = 0; d < HEX_SIDES; ++d) {
        // Create EdgeVertices from the two solid corners for this direction.
        Vector3 sc1 = getFirstSolidCorner(d);
        Vector3 sc2 = getSecondSolidCorner(d);
        Vector3 v1 = {.x = center.x + sc1.x, .y = center.y, .z = center.z + sc1.z};
        Vector3 v2 = {.x = center.x + sc2.x, .y = center.y, .z = center.z + sc2.z};

        EdgeVertices edge = makeEdge(v1, v2);

        // Triangle fan from center to subdivided edge (3 triangles).
        triangulateEdgeFan(builder, center, edge, cellColor);

        // Edge and corner connections (only NE, E, SE)
        if (d < NEAR_DIRECTION_COUNT) {
            triangulateConnection(builder, map, row, col, d, edge, cellColor, cellElev);
        }
    }
}

// ── Main draw function ───────────────────────────────────────────────────────

void drawMap(const game::Map &map, std::optional<hex::TileCoord> /*highlightedTile*/, const game::FogOfWar * /*fog*/,
             game::FactionId /*playerFactionId*/) {
    HexMeshBuilder builder;

    for (int row = 0; row < map.height(); ++row) {
        for (int col = 0; col < map.width(); ++col) {
            triangulateCell(builder, map, row, col);
        }
    }

    builder.flush();
}

} // namespace engine
