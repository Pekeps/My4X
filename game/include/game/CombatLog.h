#pragma once

#include "game/Faction.h"
#include "game/TerrainType.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace game {

/// A single combat event recording everything that happened in one engagement.
///
/// This is a pure data struct with no behavior, designed for serialization,
/// replay, UI display, and network transmission. It captures both the combatants
/// and the outcome in a single snapshot.
struct CombatEvent {
    /// Index of the attacking unit in the game state's unit list.
    std::size_t attackerUnitIndex = 0;
    /// Index of the defending unit in the game state's unit list.
    std::size_t defenderUnitIndex = 0;

    /// Faction that owns the attacker.
    FactionId attackerFactionId = 0;
    /// Faction that owns the defender.
    FactionId defenderFactionId = 0;

    /// Damage dealt to the defender by the attacker's strike.
    int damageToDefender = 0;
    /// Damage dealt to the attacker by the defender's counter-attack (0 if none).
    int damageToAttacker = 0;

    /// Whether the defender died as a result of this engagement.
    bool defenderDied = false;
    /// Whether the attacker died from the counter-attack.
    bool attackerDied = false;

    /// Turn number when this combat occurred.
    int turn = 0;

    /// Tile coordinates where the defender stood (the combat location).
    int tileRow = 0;
    int tileCol = 0;

    /// Terrain the defender was standing on (provides context for defense bonuses).
    TerrainType defenderTerrain = TerrainType::Plains;

    /// Display name of the attacking unit (captured at combat time for log display).
    std::string attackerName;
    /// Display name of the defending unit (captured at combat time for log display).
    std::string defenderName;

    /// Display name of the attacker's faction (captured at combat time for log display).
    std::string attackerFactionName;
    /// Display name of the defender's faction (captured at combat time for log display).
    std::string defenderFactionName;

    /// Whether this event represents a city capture rather than combat.
    bool isCaptureEvent = false;

    /// Name of the captured city (only valid when isCaptureEvent is true).
    std::string capturedCityName;
};

/// Bounded combat log that stores a fixed maximum number of events.
///
/// Uses a ring buffer internally so that when the log is full, the oldest
/// events are silently dropped. This prevents unbounded memory growth in long
/// games or multiplayer sessions.
///
/// The log is a game-layer concept with no rendering dependencies.
class CombatLog {
  public:
    /// Default maximum number of events the log will retain.
    static constexpr std::size_t DEFAULT_CAPACITY = 1000;

    /// Construct a log with the given maximum capacity.
    explicit CombatLog(std::size_t capacity = DEFAULT_CAPACITY);

    /// Append a combat event to the log. If the log is at capacity, the oldest
    /// event is overwritten.
    void append(const CombatEvent &event);

    /// Return the total number of events currently stored.
    [[nodiscard]] std::size_t size() const;

    /// Return the maximum number of events this log can hold.
    [[nodiscard]] std::size_t capacity() const;

    /// Return true if the log contains no events.
    [[nodiscard]] bool empty() const;

    /// Return the most recent N events, ordered oldest-to-newest.
    /// If fewer than `count` events exist, returns all of them.
    [[nodiscard]] std::vector<CombatEvent> recentEvents(std::size_t count) const;

    /// Return all events that occurred on the given turn, ordered oldest-to-newest.
    [[nodiscard]] std::vector<CombatEvent> eventsForTurn(int turn) const;

    /// Return all stored events, ordered oldest-to-newest.
    [[nodiscard]] std::vector<CombatEvent> allEvents() const;

    /// Remove all events from the log.
    void clear();

  private:
    std::vector<CombatEvent> buffer_;
    std::size_t capacity_;
    std::size_t head_ = 0;  // next write position
    std::size_t count_ = 0; // number of valid entries
};

} // namespace game
