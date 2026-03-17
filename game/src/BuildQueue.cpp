#include "game/BuildQueue.h"

namespace game {

int BuildQueue::buildingCost(BuildingType type) {
    switch (type) {
    case BuildingType::Farm:
        return makeFarm(0, 0).constructionCost().production;
    case BuildingType::Mine:
        return makeMine(0, 0).constructionCost().production;
    case BuildingType::Market:
        return makeMarket(0, 0).constructionCost().production;
    }
    return 0;
}

void BuildQueue::enqueue(BuildingType type, int targetRow, int targetCol) {
    queue_.push(BuildQueueItem{.type = type, .targetRow = targetRow, .targetCol = targetCol});
}

std::optional<BuildQueueItem> BuildQueue::currentItem() const {
    if (queue_.empty()) {
        return std::nullopt;
    }
    return queue_.front();
}

int BuildQueue::turnsRemaining(int productionPerTurn) const {
    if (queue_.empty() || productionPerTurn <= 0) {
        return 0;
    }
    const int cost = buildingCost(queue_.front().type);
    return (cost - accumulated_ + productionPerTurn - 1) / productionPerTurn;
}

int BuildQueue::accumulatedProduction() const {
    return accumulated_;
}

std::optional<Building> BuildQueue::applyProduction(int amount) {
    if (queue_.empty()) {
        return std::nullopt;
    }
    accumulated_ += amount;
    const BuildQueueItem item = queue_.front();
    if (accumulated_ >= buildingCost(item.type)) {
        queue_.pop();
        accumulated_ = 0;
        switch (item.type) {
        case BuildingType::Farm:
            return makeFarm(item.targetRow, item.targetCol);
        case BuildingType::Mine:
            return makeMine(item.targetRow, item.targetCol);
        case BuildingType::Market:
            return makeMarket(item.targetRow, item.targetCol);
        }
    }
    return std::nullopt;
}

void BuildQueue::cancel() {
    if (!queue_.empty()) {
        queue_.pop();
        accumulated_ = 0;
    }
}

bool BuildQueue::isEmpty() const {
    return queue_.empty();
}

} // namespace game
