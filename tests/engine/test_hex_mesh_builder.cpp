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

TEST(HexMeshBuilderTest, AddTrianglePerVertexColor) {
    engine::HexMeshBuilder builder;
    Vector3 v1 = {0, 0, 0}, v2 = {1, 0, 0}, v3 = {0, 0, 1};
    Color c1 = {255, 0, 0, 255}, c2 = {0, 255, 0, 255}, c3 = {0, 0, 255, 255};
    builder.addTriangle(v1, v2, v3, c1, c2, c3);
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

TEST(HexMeshBuilderTest, MultipleTrianglesAccumulate) {
    engine::HexMeshBuilder builder;
    Vector3 v = {0, 0, 0};
    Color c = {128, 128, 128, 255};
    builder.addTriangle(v, v, v, c);
    builder.addTriangle(v, v, v, c);
    builder.addTriangle(v, v, v, c);
    EXPECT_EQ(builder.triangleCount(), 3);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
