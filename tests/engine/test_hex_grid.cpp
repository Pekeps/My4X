#include "engine/HexGrid.h"

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

const float EPSILON = 0.001F;

TEST(HexGridTest, OriginTileCenter) {
    Vector3 center = engine::hex::tileCenter(0, 0);
    EXPECT_NEAR(center.x, 0.0F, EPSILON);
    EXPECT_NEAR(center.y, 0.0F, EPSILON);
    EXPECT_NEAR(center.z, 0.0F, EPSILON);
}

TEST(HexGridTest, EvenRowNoOffset) {
    // Row 2 is even, so no horizontal offset.
    Vector3 center = engine::hex::tileCenter(2, 0);
    EXPECT_NEAR(center.x, 0.0F, EPSILON);
    EXPECT_GT(center.z, 0.0F); // Moved down in Z.
}

TEST(HexGridTest, OddRowHasOffset) {
    // Row 1 is odd, so it should be shifted right compared to row 0.
    Vector3 even = engine::hex::tileCenter(0, 0);
    Vector3 odd = engine::hex::tileCenter(1, 0);
    EXPECT_GT(odd.x, even.x);
}

TEST(HexGridTest, ColumnsSpaceHorizontally) {
    Vector3 col0 = engine::hex::tileCenter(0, 0);
    Vector3 col1 = engine::hex::tileCenter(0, 1);
    Vector3 col2 = engine::hex::tileCenter(0, 2);

    float spacing = col1.x - col0.x;
    EXPECT_GT(spacing, 0.0F);
    // Spacing should be consistent between columns.
    EXPECT_NEAR(col2.x - col1.x, spacing, EPSILON);
}

TEST(HexGridTest, RowsSpaceVertically) {
    Vector3 row0 = engine::hex::tileCenter(0, 0);
    Vector3 row1 = engine::hex::tileCenter(1, 0);
    Vector3 row2 = engine::hex::tileCenter(2, 0);

    float spacing = row1.z - row0.z;
    EXPECT_GT(spacing, 0.0F);
    EXPECT_NEAR(row2.z - row1.z, spacing, EPSILON);
}

TEST(HexGridTest, TileCenterYIsZero) {
    // All tiles should be on the XZ plane.
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            Vector3 center = engine::hex::tileCenter(row, col);
            EXPECT_NEAR(center.y, 0.0F, EPSILON);
        }
    }
}

TEST(HexGridTest, HexVerticesCount) {
    Vector3 center = {.x = 0.0F, .y = 0.0F, .z = 0.0F};
    auto verts = engine::hex::hexVertices(center);
    EXPECT_EQ(verts.size(), 6);
}

TEST(HexGridTest, HexVerticesOnXZPlane) {
    Vector3 center = {.x = 5.0F, .y = 0.0F, .z = 3.0F};
    auto verts = engine::hex::hexVertices(center);
    for (const auto &vert : verts) {
        EXPECT_NEAR(vert.y, 0.0F, EPSILON);
    }
}

TEST(HexGridTest, HexVerticesEquidistantFromCenter) {
    Vector3 center = {.x = 0.0F, .y = 0.0F, .z = 0.0F};
    auto verts = engine::hex::hexVertices(center);

    const float HEX_RADIUS = 1.0F;
    for (const auto &vert : verts) {
        float dist = std::sqrt((vert.x * vert.x) + (vert.z * vert.z));
        EXPECT_NEAR(dist, HEX_RADIUS, EPSILON);
    }
}

TEST(HexGridTest, HexVerticesTranslatedByCenter) {
    Vector3 origin = {.x = 0.0F, .y = 0.0F, .z = 0.0F};
    Vector3 offset = {.x = 10.0F, .y = 0.0F, .z = 7.0F};

    auto vertsOrigin = engine::hex::hexVertices(origin);
    auto vertsOffset = engine::hex::hexVertices(offset);

    for (int i = 0; i < engine::hex::HEX_VERTEX_COUNT; ++i) {
        EXPECT_NEAR(vertsOffset[i].x - vertsOrigin[i].x, 10.0F, EPSILON);
        EXPECT_NEAR(vertsOffset[i].z - vertsOrigin[i].z, 7.0F, EPSILON);
    }
}

TEST(HexGridTest, HexEdgeLengthsEqual) {
    Vector3 center = {.x = 0.0F, .y = 0.0F, .z = 0.0F};
    auto verts = engine::hex::hexVertices(center);

    float firstEdge = -1.0F;
    for (int i = 0; i < engine::hex::HEX_VERTEX_COUNT; ++i) {
        int next = (i + 1) % engine::hex::HEX_VERTEX_COUNT;
        float diffX = verts[next].x - verts[i].x;
        float diffZ = verts[next].z - verts[i].z;
        float edge = std::sqrt((diffX * diffX) + (diffZ * diffZ));
        if (firstEdge < 0.0F) {
            firstEdge = edge;
        } else {
            EXPECT_NEAR(edge, firstEdge, EPSILON);
        }
    }
}
