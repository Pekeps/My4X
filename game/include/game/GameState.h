#pragma once

namespace game {

class GameState {
  public:
    void nextTurn();
    [[nodiscard]] int getTurn() const;

  private:
    int turn_ = 1;
};

} // namespace game
