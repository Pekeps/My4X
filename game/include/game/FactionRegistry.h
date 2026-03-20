#pragma once

#include "game/Faction.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace game {

/// Registry that owns and manages all Faction instances.
///
/// Factions are stored as value types in a vector.  Each faction receives
/// a unique, auto-incremented FactionId upon registration.  The registry
/// provides lookup by ID as well as filtered iteration by faction type.
class FactionRegistry {
  public:
    /// Create and register a new faction.  Returns the auto-assigned ID.
    FactionId addFaction(std::string name, FactionType type, std::uint8_t colorIndex);

    /// Look up a faction by ID.  Throws std::out_of_range if not found.
    [[nodiscard]] const Faction &getFaction(FactionId id) const;

    /// Look up a faction by ID.  Returns nullptr if not found.
    [[nodiscard]] const Faction *findFaction(FactionId id) const;

    /// Mutable lookup by ID.  Throws std::out_of_range if not found.
    [[nodiscard]] Faction &getMutableFaction(FactionId id);

    /// Return a const reference to all registered factions.
    [[nodiscard]] const std::vector<Faction> &allFactions() const;

    /// Return a mutable reference to all registered factions.
    [[nodiscard]] std::vector<Faction> &allMutableFactions();

    /// Return pointers to all player-controlled factions.
    [[nodiscard]] std::vector<const Faction *> playerFactions() const;

    /// Return pointers to all neutral factions (both hostile and passive).
    [[nodiscard]] std::vector<const Faction *> neutralFactions() const;

    /// Current value of the next ID counter (useful for serialization).
    [[nodiscard]] FactionId nextFactionId() const;

    /// Set the next ID counter (for deserialization).
    void setNextFactionId(FactionId id);

    /// Add a faction preserving its existing ID (for deserialization).
    void restoreFaction(Faction faction);

  private:
    std::vector<Faction> factions_;
    FactionId nextFactionId_ = 1;
};

} // namespace game
