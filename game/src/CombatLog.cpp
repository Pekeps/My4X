#include "game/CombatLog.h"

#include <algorithm>

namespace game {

CombatLog::CombatLog(std::size_t capacity) : capacity_(capacity) { buffer_.resize(capacity_); }

void CombatLog::append(const CombatEvent &event) {
    buffer_[head_] = event;
    head_ = (head_ + 1) % capacity_;
    if (count_ < capacity_) {
        ++count_;
    }
}

std::size_t CombatLog::size() const { return count_; }

std::size_t CombatLog::capacity() const { return capacity_; }

bool CombatLog::empty() const { return count_ == 0; }

std::vector<CombatEvent> CombatLog::recentEvents(std::size_t count) const {
    const std::size_t resultCount = std::min(count, count_);
    std::vector<CombatEvent> result;
    result.reserve(resultCount);

    // The most recent event is at (head_ - 1), so we want the last `resultCount`
    // events starting from (head_ - resultCount) going forward.
    // Oldest of the recent N is at offset (head_ - resultCount) within the ring.
    for (std::size_t i = 0; i < resultCount; ++i) {
        // Calculate index: start from (head_ - resultCount + i), wrapping around.
        const std::size_t idx = (head_ + capacity_ - resultCount + i) % capacity_;
        result.push_back(buffer_[idx]);
    }

    return result;
}

std::vector<CombatEvent> CombatLog::eventsForTurn(int turn) const {
    std::vector<CombatEvent> result;

    // Walk from oldest to newest to preserve chronological order.
    const std::size_t start = (head_ + capacity_ - count_) % capacity_;
    for (std::size_t i = 0; i < count_; ++i) {
        const std::size_t idx = (start + i) % capacity_;
        if (buffer_[idx].turn == turn) {
            result.push_back(buffer_[idx]);
        }
    }

    return result;
}

std::vector<CombatEvent> CombatLog::allEvents() const { return recentEvents(count_); }

void CombatLog::clear() {
    head_ = 0;
    count_ = 0;
}

} // namespace game
