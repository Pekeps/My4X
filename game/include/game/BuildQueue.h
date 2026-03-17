#pragma once
#include "game/Building.h"
#include <cstdint>
#include <optional>
#include <queue>

namespace game {

enum class BuildingType : std::uint8_t { Farm, Mine, Market };

struct BuildQueueItem {
    BuildingType type;
    int targetRow;
    int targetCol;
};

class BuildQueue {
  public:
    void enqueue(BuildingType type, int targetRow, int targetCol);
    [[nodiscard]] std::optional<BuildQueueItem> currentItem() const;
    [[nodiscard]] int turnsRemaining(int productionPerTurn) const;
    [[nodiscard]] int accumulatedProduction() const;
    // Applies production to front item. Returns completed Building if threshold reached.
    std::optional<Building> applyProduction(int amount);
    void cancel();
    [[nodiscard]] bool isEmpty() const;

  private:
    std::queue<BuildQueueItem> queue_;
    int accumulated_ = 0;
    [[nodiscard]] static int buildingCost(BuildingType type);
};

} // namespace game
