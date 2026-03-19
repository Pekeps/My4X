#pragma once
#include "game/Building.h"
#include <functional>
#include <optional>
#include <queue>
#include <string>
#include <vector>

namespace game {

using BuildingFactory = std::function<Building(int row, int col)>;

struct BuildQueueItem {
    std::string name;
    int productionCost{};
    int targetRow{};
    int targetCol{};
    BuildingFactory factory;
};

class BuildQueue {
  public:
    void enqueue(BuildingFactory factory, int targetRow, int targetCol);
    [[nodiscard]] std::optional<BuildQueueItem> currentItem() const;
    [[nodiscard]] int turnsRemaining(int productionPerTurn) const;
    [[nodiscard]] int accumulatedProduction() const;
    // Applies production to front item. Returns completed Building if threshold reached.
    std::optional<Building> applyProduction(int amount);
    void cancel();
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
