#pragma once

#include "game/Resource.h"

#include <cstdint>
#include <string>
#include <utility>

namespace game {

using FactionId = std::uint32_t;

/// Distinguishes player-controlled factions from AI/neutral ones.
enum class FactionType : std::uint8_t {
    Player,
    NeutralHostile,
    NeutralPassive,
};

/// Represents a player or AI-controlled faction in the game.
///
/// Each faction has a unique ID, display name, type classification,
/// a color index for rendering, and a resource stockpile.
/// This class lives in the game layer and has no engine/raylib dependencies.
class Faction {
  public:
    Faction(FactionId id, std::string name, FactionType type, std::uint8_t colorIndex);

    [[nodiscard]] FactionId id() const;
    [[nodiscard]] const std::string &name() const;
    [[nodiscard]] FactionType type() const;
    [[nodiscard]] std::uint8_t colorIndex() const;

    void setName(std::string name);
    void setColorIndex(std::uint8_t colorIndex);

    [[nodiscard]] const Resource &stockpile() const;
    [[nodiscard]] Resource &stockpile();

    /// Add resources to the stockpile.
    void addResources(const Resource &amount);

    /// Attempt to spend resources. Returns true and deducts if the
    /// stockpile has enough; returns false and leaves it unchanged otherwise.
    bool spendResources(const Resource &cost);

  private:
    FactionId id_;
    std::string name_;
    FactionType type_;
    std::uint8_t colorIndex_;
    Resource stockpile_;
};

} // namespace game
