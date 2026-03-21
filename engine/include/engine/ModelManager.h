#pragma once

#include "raylib.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace engine {

/// Procedural shape types for fallback model generation.
enum class FallbackShape : std::uint8_t {
    Cube,     ///< Default cube shape.
    Cone,     ///< Cone / pyramid shape (e.g., archers).
    Sphere,   ///< Sphere shape (e.g., settlers).
    Cylinder, ///< Cylinder shape (e.g., scouts).
};

/// Manages loading, caching, and lifetime of 3D models.
///
/// Each model is stored under a unique string key. Loading the same key twice
/// is a no-op (returns true and reuses the cached model). Missing files are
/// handled gracefully by substituting a procedural fallback cube.
///
/// IMPORTANT: Loading actual Model objects (loadModel, generateFallback)
/// requires an active raylib window context. The key/lookup logic (hasModel,
/// modelCount, keys) works without one.
class ModelManager {
  public:
    ModelManager() = default;
    ~ModelManager();

    // Non-copyable (Model resources are GPU-bound).
    ModelManager(const ModelManager &) = delete;
    ModelManager &operator=(const ModelManager &) = delete;

    // Movable.
    ModelManager(ModelManager &&other) noexcept;
    ModelManager &operator=(ModelManager &&other) noexcept;

    /// Load a GLTF/GLB model from `filePath` and store it under `key`.
    /// Returns true on success, false if the file could not be loaded
    /// (a fallback placeholder is stored instead).
    /// If `key` already exists, returns true without reloading.
    bool loadModel(const std::string &key, const std::string &filePath);

    /// Generate a procedural fallback model (cube) and store it under `key`.
    /// Useful for prototyping when no GLTF assets exist yet.
    /// If `key` already exists, does nothing and returns true.
    bool generateFallback(const std::string &key);

    /// Generate a procedural fallback model of the given shape and store it
    /// under `key`. If `key` already exists, does nothing and returns true.
    bool generateFallback(const std::string &key, FallbackShape shape);

    /// Retrieve a loaded model by key.
    /// Returns a pointer to the Model, or nullptr if not found.
    [[nodiscard]] const Model *getModel(const std::string &key) const;

    /// Check whether a model is registered under the given key.
    [[nodiscard]] bool hasModel(const std::string &key) const;

    /// Return the number of currently loaded models.
    [[nodiscard]] std::size_t modelCount() const;

    /// Unload all models and clear the registry.
    void unloadAll();

  private:
    /// Internal storage mapping keys to raylib Model objects.
    std::unordered_map<std::string, Model> models_;
};

} // namespace engine
