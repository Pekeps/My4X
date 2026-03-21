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
    void addQuad(Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4, Color c1, Color c2, Color c3, Color c4);

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
