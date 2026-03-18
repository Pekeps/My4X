#include "game/BuildQueue.h"

namespace game {

void BuildQueue::enqueue(BuildingFactory factory, int targetRow, int targetCol) {
    auto prototype = factory(0, 0);
    queue_.push(BuildQueueItem{.name = prototype.name(),
                               .productionCost = prototype.constructionCost().production,
                               .targetRow = targetRow,
                               .targetCol = targetCol,
                               .factory = std::move(factory)});
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
    const int cost = queue_.front().productionCost;
    return (cost - accumulated_ + productionPerTurn - 1) / productionPerTurn;
}

int BuildQueue::accumulatedProduction() const { return accumulated_; }

std::optional<Building> BuildQueue::applyProduction(int amount) {
    if (queue_.empty()) {
        return std::nullopt;
    }
    accumulated_ += amount;
    const auto &item = queue_.front();
    if (accumulated_ >= item.productionCost) {
        auto building = item.factory(item.targetRow, item.targetCol);
        queue_.pop();
        accumulated_ = 0;
        return building;
    }
    return std::nullopt;
}

void BuildQueue::cancel() {
    if (!queue_.empty()) {
        queue_.pop();
        accumulated_ = 0;
    }
}

bool BuildQueue::isEmpty() const { return queue_.empty(); }

std::vector<BuildQueueItem> BuildQueue::allItems() const {
    std::vector<BuildQueueItem> result;
    auto copy = queue_;
    while (!copy.empty()) {
        result.push_back(std::move(copy.front()));
        copy.pop();
    }
    return result;
}

void BuildQueue::restoreItem(BuildQueueItem item) { queue_.push(std::move(item)); }

void BuildQueue::setAccumulatedProduction(int amount) { accumulated_ = amount; }

} // namespace game
