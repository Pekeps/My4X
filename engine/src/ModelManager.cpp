#include "engine/ModelManager.h"

#include "raylib.h"

#include <utility>

namespace engine {

// ── Fallback mesh dimensions ────────────────────────────────────────────────

/// Side length of the procedural fallback cube used when a model file is missing.
static constexpr float FALLBACK_CUBE_SIZE = 1.0F;

/// Radius for the procedural fallback cone mesh.
static constexpr float FALLBACK_CONE_RADIUS = 0.5F;
/// Height for the procedural fallback cone mesh.
static constexpr float FALLBACK_CONE_HEIGHT = 1.0F;
/// Number of slices for the procedural fallback cone mesh.
static constexpr int FALLBACK_CONE_SLICES = 8;

/// Radius for the procedural fallback sphere mesh.
static constexpr float FALLBACK_SPHERE_RADIUS = 0.5F;
/// Number of rings for the procedural fallback sphere mesh.
static constexpr int FALLBACK_SPHERE_RINGS = 8;
/// Number of slices for the procedural fallback sphere mesh.
static constexpr int FALLBACK_SPHERE_SLICES = 8;

/// Radius for the procedural fallback cylinder mesh.
static constexpr float FALLBACK_CYLINDER_RADIUS = 0.3F;
/// Height for the procedural fallback cylinder mesh.
static constexpr float FALLBACK_CYLINDER_HEIGHT = 1.0F;
/// Number of slices for the procedural fallback cylinder mesh.
static constexpr int FALLBACK_CYLINDER_SLICES = 8;

// ── Destructor ──────────────────────────────────────────────────────────────

ModelManager::~ModelManager() { unloadAll(); }

// ── Move operations ─────────────────────────────────────────────────────────

ModelManager::ModelManager(ModelManager &&other) noexcept : models_(std::move(other.models_)) {}

ModelManager &ModelManager::operator=(ModelManager &&other) noexcept {
    if (this != &other) {
        unloadAll();
        models_ = std::move(other.models_);
    }
    return *this;
}

// ── loadModel ───────────────────────────────────────────────────────────────

bool ModelManager::loadModel(const std::string &key, const std::string &filePath) {
    // Already cached — nothing to do.
    if (models_.contains(key)) {
        return true;
    }

    // Check if the file exists before asking raylib to load it.
    if (!FileExists(filePath.c_str())) {
        TraceLog( // NOLINT(cppcoreguidelines-pro-type-vararg)
            LOG_WARNING, "ModelManager: file not found '%s', using fallback for key '%s'", filePath.c_str(),
            key.c_str());
        return generateFallback(key);
    }

    Model model = LoadModel(filePath.c_str());

    // raylib returns a model with meshCount == 0 on failure.
    if (model.meshCount <= 0) {
        TraceLog( // NOLINT(cppcoreguidelines-pro-type-vararg)
            LOG_WARNING, "ModelManager: failed to load '%s', using fallback for key '%s'", filePath.c_str(),
            key.c_str());
        return generateFallback(key);
    }

    models_.emplace(key, model);
    TraceLog(LOG_INFO, "ModelManager: loaded '%s' as '%s' (%d meshes)", // NOLINT(cppcoreguidelines-pro-type-vararg)
             filePath.c_str(), key.c_str(), model.meshCount);
    return true;
}

// ── generateFallback ────────────────────────────────────────────────────────

bool ModelManager::generateFallback(const std::string &key) { return generateFallback(key, FallbackShape::Cube); }

bool ModelManager::generateFallback(const std::string &key, FallbackShape shape) {
    if (models_.contains(key)) {
        return true;
    }

    Mesh mesh{};
    const char *shapeName = "cube";

    switch (shape) {
    case FallbackShape::Cone:
        mesh = GenMeshCone(FALLBACK_CONE_RADIUS, FALLBACK_CONE_HEIGHT, FALLBACK_CONE_SLICES);
        shapeName = "cone";
        break;
    case FallbackShape::Sphere:
        mesh = GenMeshSphere(FALLBACK_SPHERE_RADIUS, FALLBACK_SPHERE_RINGS, FALLBACK_SPHERE_SLICES);
        shapeName = "sphere";
        break;
    case FallbackShape::Cylinder:
        mesh = GenMeshCylinder(FALLBACK_CYLINDER_RADIUS, FALLBACK_CYLINDER_HEIGHT, FALLBACK_CYLINDER_SLICES);
        shapeName = "cylinder";
        break;
    case FallbackShape::Cube:
    default:
        mesh = GenMeshCube(FALLBACK_CUBE_SIZE, FALLBACK_CUBE_SIZE, FALLBACK_CUBE_SIZE);
        shapeName = "cube";
        break;
    }

    Model model = LoadModelFromMesh(mesh);
    models_.emplace(key, model);

    TraceLog(LOG_INFO, "ModelManager: generated fallback %s for key '%s'", // NOLINT(cppcoreguidelines-pro-type-vararg)
             shapeName, key.c_str());
    return true;
}

// ── getModel ────────────────────────────────────────────────────────────────

const Model *ModelManager::getModel(const std::string &key) const {
    auto iter = models_.find(key);
    if (iter == models_.end()) {
        return nullptr;
    }
    return &iter->second;
}

// ── hasModel ────────────────────────────────────────────────────────────────

bool ModelManager::hasModel(const std::string &key) const { return models_.contains(key); }

// ── modelCount ──────────────────────────────────────────────────────────────

std::size_t ModelManager::modelCount() const { return models_.size(); }

// ── unloadAll ───────────────────────────────────────────────────────────────

void ModelManager::unloadAll() {
    for (auto &[key, model] : models_) {
        UnloadModel(model);
        TraceLog(LOG_INFO, "ModelManager: unloaded model '%s'", // NOLINT(cppcoreguidelines-pro-type-vararg)
                 key.c_str());
    }
    models_.clear();
}

} // namespace engine
