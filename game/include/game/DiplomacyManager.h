#pragma once

#include "game/Faction.h"

#include <cstdint>
#include <map>
#include <utility>

namespace game {

class FactionRegistry;

/// Diplomatic relationship between two factions.
enum class DiplomacyState : std::uint8_t {
    War,
    Peace,
    Alliance,
};

/// Manages pairwise diplomatic relationships between factions.
///
/// Relationships are symmetric: relation(A, B) always equals relation(B, A).
/// Internally, pairs are stored with the smaller FactionId first to ensure
/// a single canonical key per pair.
class DiplomacyManager {
  public:
    /// Get the diplomatic relationship between two factions.
    /// Returns Peace if no relationship has been explicitly set and no
    /// default rule applies.  Querying a faction against itself returns Peace.
    [[nodiscard]] DiplomacyState getRelation(FactionId factionA, FactionId factionB) const;

    /// Set the diplomatic relationship between two factions.
    /// The relationship is symmetric: setRelation(A, B, s) implies
    /// getRelation(B, A) == s.  Setting a relation for a faction with
    /// itself is a no-op.
    void setRelation(FactionId factionA, FactionId factionB, DiplomacyState state);

    /// Convenience: returns true if the two factions are at war.
    [[nodiscard]] bool areAtWar(FactionId factionA, FactionId factionB) const;

    /// Convenience: returns true if the two factions are allied.
    [[nodiscard]] bool areAllied(FactionId factionA, FactionId factionB) const;

    /// Initialize default diplomatic relations for all registered factions
    /// based on their FactionType:
    ///   - Player vs Player:           War  (free-for-all)
    ///   - Player vs NeutralHostile:   War
    ///   - Player vs NeutralPassive:   Peace
    ///   - NeutralHostile vs NeutralPassive: War
    ///   - NeutralHostile vs NeutralHostile: Peace
    ///   - NeutralPassive vs NeutralPassive: Peace
    void initializeDefaults(const FactionRegistry &registry);

    /// Return the number of explicitly stored pairwise relations.
    [[nodiscard]] std::size_t size() const;

    /// Clear all stored relations.
    void clear();

  private:
    using FactionPair = std::pair<FactionId, FactionId>;

    /// Build a canonical key with the smaller ID first.
    [[nodiscard]] static FactionPair makeKey(FactionId a, FactionId b);

    /// Determine the default relation between two faction types.
    [[nodiscard]] static DiplomacyState defaultRelation(FactionType typeA, FactionType typeB);

    std::map<FactionPair, DiplomacyState> relations_;
};

} // namespace game
