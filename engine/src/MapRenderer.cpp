#include "engine/MapRenderer.h"

#include "engine/EdgeVertices.h"
#include "engine/HexEdgeType.h"
#include "engine/HexGrid.h"
#include "engine/HexMeshBuilder.h"
#include "engine/HexMetrics.h"
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

/// Edge terraces between two cells (slope connection).
static void triangulateEdgeTerraces(HexMeshBuilder &builder, Vector3 beginLeft, Vector3 beginRight, Color beginColor,
                                    Vector3 endLeft, Vector3 endRight, Color endColor) {
    Vector3 v3 = terraceLerp(beginLeft, endLeft, 1);
    Vector3 v4 = terraceLerp(beginRight, endRight, 1);
    Color c2 = terraceLerpColor(beginColor, endColor, 1);

    builder.addQuad(beginLeft, beginRight, v4, v3, beginColor, beginColor, c2, c2);

    for (int i = 2; i < hex_metrics::TERRACE_INTERPOLATION_STEPS; ++i) {
        Vector3 v1 = v3;
        Vector3 v2 = v4;
        Color c1 = c2;
        v3 = terraceLerp(beginLeft, endLeft, i);
        v4 = terraceLerp(beginRight, endRight, i);
        c2 = terraceLerpColor(beginColor, endColor, i);
        builder.addQuad(v1, v2, v4, v3, c1, c1, c2, c2);
    }

    builder.addQuad(v3, v4, endRight, endLeft, c2, c2, endColor, endColor);
}

// ── Corner triangulation (Part 3 — all cases) ───────────────────────────────
// NOLINTBEGIN(readability-suspicious-call-argument)

static void triangulateCornerTerraces(HexMeshBuilder &builder, Vector3 begin, Color beginColor, Vector3 left,
                                      Color leftColor, Vector3 right, Color rightColor) {
    Vector3 v3 = terraceLerp(begin, left, 1);
    Vector3 v4 = terraceLerp(begin, right, 1);
    Color c3 = terraceLerpColor(beginColor, leftColor, 1);
    Color c4 = terraceLerpColor(beginColor, rightColor, 1);

    builder.addTriangle(begin, v3, v4, beginColor, c3, c4);

    for (int i = 2; i < hex_metrics::TERRACE_INTERPOLATION_STEPS; ++i) {
        Vector3 v1 = v3;
        Vector3 v2 = v4;
        Color c1 = c3;
        Color c2 = c4;
        v3 = terraceLerp(begin, left, i);
        v4 = terraceLerp(begin, right, i);
        c3 = terraceLerpColor(beginColor, leftColor, i);
        c4 = terraceLerpColor(beginColor, rightColor, i);
        builder.addQuad(v1, v2, v4, v3, c1, c2, c4, c3);
    }

    builder.addQuad(v3, v4, right, left, c3, c4, rightColor, leftColor);
}

static void triangulateBoundaryTriangle(HexMeshBuilder &builder, Vector3 begin, Color beginColor, Vector3 left,
                                        Color leftColor, Vector3 boundary, Color boundaryColor) {
    Vector3 v2 = terraceLerp(begin, left, 1);
    Color c2 = terraceLerpColor(beginColor, leftColor, 1);

    builder.addTriangle(begin, v2, boundary, beginColor, c2, boundaryColor);

    for (int i = 2; i < hex_metrics::TERRACE_INTERPOLATION_STEPS; ++i) {
        Vector3 v1 = v2;
        Color c1 = c2;
        v2 = terraceLerp(begin, left, i);
        c2 = terraceLerpColor(beginColor, leftColor, i);
        builder.addTriangle(v1, v2, boundary, c1, c2, boundaryColor);
    }

    builder.addTriangle(v2, left, boundary, c2, leftColor, boundaryColor);
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
        builder.addTriangle(left, right, boundary, leftColor, rightColor, boundaryColor);
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
        builder.addTriangle(left, right, boundary, leftColor, rightColor, boundaryColor);
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
        builder.addTriangle(bottom, left, right, bottomColor, leftColor, rightColor);
    }
}

// NOLINTEND(readability-suspicious-call-argument)

// ── Connection logic (Part 2 + Part 3) ───────────────────────────────────────

/// Build the bridge quad and corner triangle for one direction.
/// Called only for NE, E, SE (d=0,1,2).
// NOLINTBEGIN(readability-suspicious-call-argument)
static void triangulateConnection(HexMeshBuilder &builder, const game::Map &map, int row, int col, int dirIdx,
                                  Vector3 v1, Vector3 v2, Color cellColor, int cellElev) {
    auto dir = game::ALL_DIRECTIONS.at(dirIdx);
    auto neighborOpt = game::neighborCoord(row, col, dir, map.height(), map.width());
    if (!neighborOpt) {
        return;
    }
    auto [nr, nc] = *neighborOpt;
    Color neighborColor = getCellColor(map, nr, nc);
    int neighborElev = map.tile(nr, nc).elevation();

    // Bridge quad: v3 = v1 + bridge, v4 = v2 + bridge, with neighbor's Y.
    Vector3 bridge = getBridge(dirIdx);
    Vector3 v3 = {.x = v1.x + bridge.x, .y = elevationY(map, nr, nc), .z = v1.z + bridge.z};
    Vector3 v4 = {.x = v2.x + bridge.x, .y = elevationY(map, nr, nc), .z = v2.z + bridge.z};

    auto edgeType = classifyEdge(cellElev, neighborElev);
    if (edgeType == HexEdgeType::Slope) {
        triangulateEdgeTerraces(builder, v1, v2, cellColor, v3, v4, neighborColor);
    } else {
        builder.addQuad(v1, v2, v4, v3, cellColor, cellColor, neighborColor, neighborColor);
    }

    // Corner triangle: only for NE (d=0) and E (d=1).
    int nextD = (dirIdx + 1) % HEX_SIDES;
    auto nextDir = game::ALL_DIRECTIONS.at(nextD);
    auto nextNeighborOpt = game::neighborCoord(row, col, nextDir, map.height(), map.width());
    if (dirIdx <= 1 && nextNeighborOpt) {
        auto [nnr, nnc] = *nextNeighborOpt;
        Color nextNeighborColor = getCellColor(map, nnr, nnc);
        int nextNeighborElev = map.tile(nnr, nnc).elevation();

        // v5 = v2 + bridge(nextDirection), with next neighbor's Y.
        Vector3 nextBridge = getBridge(nextD);
        Vector3 v5 = {.x = v2.x + nextBridge.x, .y = elevationY(map, nnr, nnc), .z = v2.z + nextBridge.z};

        // Determine bottom cell and dispatch.
        if (cellElev <= neighborElev) {
            if (cellElev <= nextNeighborElev) {
                triangulateCorner(builder, v2, cellColor, cellElev, v4, neighborColor, neighborElev, v5,
                                  nextNeighborColor, nextNeighborElev);
            } else {
                triangulateCorner(builder, v5, nextNeighborColor, nextNeighborElev, v2, cellColor, cellElev, v4,
                                  neighborColor, neighborElev);
            }
        } else if (neighborElev <= nextNeighborElev) {
            triangulateCorner(builder, v4, neighborColor, neighborElev, v5, nextNeighborColor, nextNeighborElev, v2,
                              cellColor, cellElev);
        } else {
            triangulateCorner(builder, v5, nextNeighborColor, nextNeighborElev, v2, cellColor, cellElev, v4,
                              neighborColor, neighborElev);
        }
    }
}

// NOLINTEND(readability-suspicious-call-argument)

// ── Per-cell triangulation ───────────────────────────────────────────────────

static void triangulateCell(HexMeshBuilder &builder, const game::Map &map, int row, int col) {
    Vector3 center = hex::tileCenter(row, col);
    center.y = elevationY(map, row, col);
    Color cellColor = getCellColor(map, row, col);
    int cellElev = map.tile(row, col).elevation();

    for (int d = 0; d < HEX_SIDES; ++d) {
        // Solid inner triangle: center → v1 → v2
        Vector3 sc1 = getFirstSolidCorner(d);
        Vector3 sc2 = getSecondSolidCorner(d);
        Vector3 v1 = {.x = center.x + sc1.x, .y = center.y, .z = center.z + sc1.z};
        Vector3 v2 = {.x = center.x + sc2.x, .y = center.y, .z = center.z + sc2.z};

        builder.addTriangle(center, v1, v2, cellColor);

        // Edge and corner connections (only NE, E, SE)
        if (d < NEAR_DIRECTION_COUNT) {
            triangulateConnection(builder, map, row, col, d, v1, v2, cellColor, cellElev);
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
