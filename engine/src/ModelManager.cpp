#include "engine/ModelManager.h"

#include "raylib.h"

#include <utility>

namespace engine {

// ── Fallback mesh dimensions ────────────────────────────────────────────────

/// Side length of the procedural fallback cube used when a model file is missing.
static constexpr float FALLBACK_CUBE_SIZE = 1.0F;

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

bool ModelManager::generateFallback(const std::string &key) {
    if (models_.contains(key)) {
        return true;
    }

    Mesh mesh = GenMeshCube(FALLBACK_CUBE_SIZE, FALLBACK_CUBE_SIZE, FALLBACK_CUBE_SIZE);
    Model model = LoadModelFromMesh(mesh);
    models_.emplace(key, model);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    TraceLog(LOG_INFO, "ModelManager: generated fallback cube for key '%s'", key.c_str());
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
