#include "engine/HexMeshBuilder.h"

#include "rlgl.h"

#include <algorithm>
#include <cmath>

namespace engine {

static constexpr int VERTICES_PER_TRIANGLE = 3;

/// Directional light pointing down and slightly to the south-east.
static constexpr float SUN_DIR_X = 0.3F;
static constexpr float SUN_DIR_Y = -0.8F;
static constexpr float SUN_DIR_Z = 0.2F;

/// Ambient light level (minimum brightness for faces facing away from light).
static constexpr float AMBIENT_LIGHT = 0.55F;

/// Diffuse light strength (added on top of ambient for lit faces).
static constexpr float DIFFUSE_LIGHT = 0.45F;

/// Maximum total brightness.
static constexpr float MAX_BRIGHTNESS = 1.0F;

/// Max vertices per rlBegin/rlEnd batch, well under rlgl's 8192 default limit.
/// Must be a multiple of 3 (complete triangles).
static constexpr std::size_t BATCH_VERTEX_LIMIT = 6000;

void HexMeshBuilder::addTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color c1, Color c2, Color c3) {
    if (flipWinding_) {
        vertices_.push_back({v1, c1});
        vertices_.push_back({v3, c3});
        vertices_.push_back({v2, c2});
    } else {
        vertices_.push_back({v1, c1});
        vertices_.push_back({v2, c2});
        vertices_.push_back({v3, c3});
    }
}

void HexMeshBuilder::addTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color color) {
    addTriangle(v1, v2, v3, color, color, color);
}

void HexMeshBuilder::addQuad(Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4, Color c1, Color c2, Color c3, Color c4) {
    addTriangle(v1, v2, v3, c1, c2, c3);
    addTriangle(v1, v3, v4, c1, c3, c4);
}

void HexMeshBuilder::setFlipWinding(bool flip) { flipWinding_ = flip; }

void HexMeshBuilder::flush() {
    if (vertices_.empty()) {
        return;
    }

    // Normalize the sun direction.
    float sunLen = sqrtf((SUN_DIR_X * SUN_DIR_X) + (SUN_DIR_Y * SUN_DIR_Y) + (SUN_DIR_Z * SUN_DIR_Z));
    float sx = SUN_DIR_X / sunLen;
    float sy = SUN_DIR_Y / sunLen;
    float sz = SUN_DIR_Z / sunLen;

    // Apply directional lighting per triangle.
    for (std::size_t i = 0; (i + 2) < vertices_.size(); i += VERTICES_PER_TRIANGLE) {
        auto &a = vertices_[i];
        auto &b = vertices_[i + 1];
        auto &c = vertices_[i + 2];

        float e1x = b.position.x - a.position.x;
        float e1y = b.position.y - a.position.y;
        float e1z = b.position.z - a.position.z;
        float e2x = c.position.x - a.position.x;
        float e2y = c.position.y - a.position.y;
        float e2z = c.position.z - a.position.z;

        float nx = (e1y * e2z) - (e1z * e2y);
        float ny = (e1z * e2x) - (e1x * e2z);
        float nz = (e1x * e2y) - (e1y * e2x);

        float nLen = sqrtf((nx * nx) + (ny * ny) + (nz * nz));
        if (nLen > 0.0F) {
            nx /= nLen;
            ny /= nLen;
            nz /= nLen;
        }

        float dot = std::abs((-sx * nx) + (-sy * ny) + (-sz * nz));
        float brightness = AMBIENT_LIGHT + (std::min(dot, MAX_BRIGHTNESS) * DIFFUSE_LIGHT);
        brightness = std::min(brightness, MAX_BRIGHTNESS);

        auto scale = [brightness](Color &col) {
            col.r = static_cast<unsigned char>(static_cast<float>(col.r) * brightness);
            col.g = static_cast<unsigned char>(static_cast<float>(col.g) * brightness);
            col.b = static_cast<unsigned char>(static_cast<float>(col.b) * brightness);
        };
        scale(a.color);
        scale(b.color);
        scale(c.color);
    }

    // Render in small batches. Disable backface culling before EACH batch
    // to handle rlgl's internal state resets during auto-flush.
    for (std::size_t offset = 0; offset < vertices_.size();) {
        std::size_t remaining = vertices_.size() - offset;
        std::size_t batchSize = std::min(remaining, BATCH_VERTEX_LIMIT);
        batchSize = (batchSize / VERTICES_PER_TRIANGLE) * VERTICES_PER_TRIANGLE;
        if (batchSize == 0) {
            break;
        }

        // Force flush any pending rlgl draw calls, then disable culling.
        rlDrawRenderBatchActive();
        rlDisableBackfaceCulling();

        rlBegin(RL_TRIANGLES);
        for (std::size_t i = offset; i < offset + batchSize; ++i) {
            const auto &vert = vertices_[i];
            rlColor4ub(vert.color.r, vert.color.g, vert.color.b, vert.color.a);
            rlVertex3f(vert.position.x, vert.position.y, vert.position.z);
        }
        rlEnd();

        offset += batchSize;
    }

    // Restore culling for other renderers.
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    vertices_.clear();
}

void HexMeshBuilder::clear() { vertices_.clear(); }

std::size_t HexMeshBuilder::triangleCount() const { return vertices_.size() / VERTICES_PER_TRIANGLE; }

} // namespace engine
