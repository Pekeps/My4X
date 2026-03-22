#pragma once

#include "raylib.h"

#include <vector>

namespace engine {

/// Accumulates colored triangles and renders them in one rlgl batch.
/// Usage: call addTriangle() repeatedly, then flush() to render.
/// Applies directional lighting during flush based on face normals.
class HexMeshBuilder {
  public:
    /// Add a triangle with per-vertex positions and colors.
    /// When flipWinding is true, v2 and v3 are swapped to reverse winding.
    void addTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color c1, Color c2, Color c3);

    /// Add a triangle with a single uniform color (convenience).
    void addTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color color);

    /// Add a quad as two triangles (v1-v2-v3, v1-v3-v4) with per-vertex colors.
    void addQuad(Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4, Color c1, Color c2, Color c3, Color c4);

    /// Enable/disable winding flip. When enabled, all addTriangle/addQuad calls
    /// swap the 2nd and 3rd vertices, reversing face winding order.
    /// Used for corner triangles which need CW winding to match raylib's convention.
    void setFlipWinding(bool flip);

    /// Render all accumulated triangles via rlgl and clear the buffers.
    /// Applies directional lighting based on face normals before rendering.
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
    bool flipWinding_{false};
};

} // namespace engine
