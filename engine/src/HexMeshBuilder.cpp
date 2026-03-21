#include "engine/HexMeshBuilder.h"

#include "rlgl.h"

namespace engine {

static constexpr int VERTICES_PER_TRIANGLE = 3;

void HexMeshBuilder::addTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color c1, Color c2, Color c3) {
    vertices_.push_back({v1, c1});
    vertices_.push_back({v2, c2});
    vertices_.push_back({v3, c3});
}

void HexMeshBuilder::addTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color color) {
    addTriangle(v1, v2, v3, color, color, color);
}

void HexMeshBuilder::addQuad(Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4, Color c1, Color c2, Color c3, Color c4) {
    addTriangle(v1, v2, v3, c1, c2, c3);
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

void HexMeshBuilder::clear() { vertices_.clear(); }

std::size_t HexMeshBuilder::triangleCount() const { return vertices_.size() / VERTICES_PER_TRIANGLE; }

} // namespace engine
