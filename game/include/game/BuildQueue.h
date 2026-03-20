#pragma once

#include "game/Building.h"
#include "game/UnitTemplate.h"

#include <functional>
#include <optional>
#include <queue>
#include <string>
#include <variant>
#include <vector>

namespace game {

using BuildingFactory = std::function<Building(int row, int col)>;

/// Distinguishes between building and unit production orders.
enum class BuildOrderType : std::uint8_t {
    Building,
    Unit,
};

struct BuildQueueItem {
    std::string name;
    int productionCost{};

    /// Target tile coordinates (used for building placement).
    int targetRow{};
    int targetCol{};

    /// The type of production order.
    BuildOrderType orderType{BuildOrderType::Building};

    /// Factory for building orders (only valid when orderType == Building).
    BuildingFactory factory;

    /// Template key for unit orders (only valid when orderType == Unit).
    std::string templateKey;
};

/// Result of completing a build queue item — either a Building or a unit spawn request.
struct UnitSpawnRequest {
    std::string templateKey;
};

using BuildQueueResult = std::variant<Building, UnitSpawnRequest>;

class BuildQueue {
  public:
    void enqueue(BuildingFactory factory, int targetRow, int targetCol);

    /// Enqueue a unit production order.
    void enqueueUnit(const std::string &templateKey, int productionCost);

    [[nodiscard]] std::optional<BuildQueueItem> currentItem() const;
    [[nodiscard]] int turnsRemaining(int productionPerTurn) const;
    [[nodiscard]] int accumulatedProduction() const;

    /// Applies production to front item. Returns completed result if threshold reached.
    std::optional<BuildQueueResult> applyProduction(int amount);

    void cancel();

    /// Remove all items from the queue and reset accumulated production.
    void clear();

    [[nodiscard]] bool isEmpty() const;

    /// Returns a copy of all queued items (for serialization).
    [[nodiscard]] std::vector<BuildQueueItem> allItems() const;

    /// Restore a single item to the back of the queue (for deserialization).
    void restoreItem(BuildQueueItem item);

    /// Set accumulated production directly (for deserialization).
    void setAccumulatedProduction(int amount);

  private:
    std::queue<BuildQueueItem> queue_;
    int accumulated_ = 0;
};

} // namespace game
