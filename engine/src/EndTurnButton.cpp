#include "engine/EndTurnButton.h"

#include <string>

namespace engine {

/// Divisor used to compute center positions (halving).
static constexpr float HALF = 2.0F;

static Rectangle computeButtonBounds(int screenWidth, int screenHeight) {
    auto x = (static_cast<float>(screenWidth) / HALF) - (static_cast<float>(EndTurnButton::WIDTH) / HALF);
    auto y = static_cast<float>(screenHeight - EndTurnButton::BOTTOM_MARGIN - EndTurnButton::HEIGHT);
    return {.x = x,
            .y = y,
            .width = static_cast<float>(EndTurnButton::WIDTH),
            .height = static_cast<float>(EndTurnButton::HEIGHT)};
}

bool EndTurnButton::update(int screenWidth, int screenHeight) {
    Rectangle bounds = computeButtonBounds(screenWidth, screenHeight);
    auto mx = static_cast<float>(GetMouseX());
    auto my = static_cast<float>(GetMouseY());
    hovered_ = mx >= bounds.x && mx <= (bounds.x + bounds.width) && my >= bounds.y && my <= (bounds.y + bounds.height);
    pressed_ = hovered_ && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    return hovered_ && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

void EndTurnButton::draw(int screenWidth, int screenHeight, int currentTurn) const {
    Rectangle bounds = computeButtonBounds(screenWidth, screenHeight);

    // Draw turn counter label above the button.
    std::string turnStr = "Turn " + std::to_string(currentTurn);
    int turnW = MeasureText(turnStr.c_str(), TURN_FONT_SIZE);
    auto boundsX = static_cast<int>(bounds.x);
    auto boundsW = static_cast<int>(bounds.width);
    auto boundsY = static_cast<int>(bounds.y);
    auto boundsH = static_cast<int>(bounds.height);
    int turnX = boundsX + ((boundsW - turnW) / 2);
    int turnY = boundsY - TURN_FONT_SIZE - TURN_LABEL_GAP;
    DrawText(turnStr.c_str(), turnX, turnY, TURN_FONT_SIZE, TURN_TEXT_COLOR);

    // Draw button.
    Color bgCol = BG_COLOR;
    if (pressed_) {
        bgCol = PRESS_BG;
    } else if (hovered_) {
        bgCol = HOVER_BG;
    }

    DrawRectangleRounded(bounds, ROUNDNESS, SEGMENTS, bgCol);
    DrawRectangleRoundedLines(bounds, ROUNDNESS, SEGMENTS, BORDER_COLOR);

    const char *label = "End Turn";
    int labelW = MeasureText(label, FONT_SIZE);
    int labelX = boundsX + ((boundsW - labelW) / 2);
    int labelY = boundsY + ((boundsH - FONT_SIZE) / 2);
    DrawText(label, labelX, labelY, FONT_SIZE, TEXT_COLOR);
}

} // namespace engine
