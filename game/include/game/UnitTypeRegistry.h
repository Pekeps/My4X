#pragma once

#include "game/UnitTemplate.h"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace game {

/// Registry of all known unit types.
///
/// Populated once at game initialisation with UnitTemplate instances keyed by
/// a string identifier.  The registry owns the templates and hands out const
/// references — templates are immutable after registration.
class UnitTypeRegistry {
  public:
    /// Register a unit template under the given key.  Throws if the key is
    /// already registered.
    void registerTemplate(const std::string &key, UnitTemplate tmpl);

    /// Retrieve a registered template by key.  Throws std::out_of_range if
    /// the key is not found.
    [[nodiscard]] const UnitTemplate &getTemplate(const std::string &key) const;

    /// Check whether a template with the given key exists.
    [[nodiscard]] bool hasTemplate(const std::string &key) const;

    /// Return all registered template keys.
    [[nodiscard]] std::vector<std::string> allKeys() const;

    /// Populate the registry with the built-in (hardcoded) unit types used
    /// by the base game.
    void registerDefaults();

  private:
    std::unordered_map<std::string, UnitTemplate> templates_;
};

} // namespace game
