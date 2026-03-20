#include "engine/MenuSystem.h"

#include <string>

namespace engine {

/// Divisor used to compute center positions (halving).
static constexpr float HALF = 2.0F;

// ── Helper: check if mouse is inside a rectangle ────────────────────────────

static bool isMouseOver(const Rectangle &rect) {
    auto mx = static_cast<float>(GetMouseX());
    auto my = static_cast<float>(GetMouseY());
    return mx >= rect.x && mx <= (rect.x + rect.width) && my >= rect.y && my <= (rect.y + rect.height);
}

// ── Helper: draw a styled button ────────────────────────────────────────────

static constexpr float BUTTON_DRAW_ROUNDNESS = 0.2F;
static constexpr int BUTTON_DRAW_SEGMENTS = 6;

static void drawStyledButton(const MenuButton &btn, int fontSize, Color bg, Color hoverBg, Color pressBg,
                             Color borderCol, Color textCol, Color textDimCol) {
    Color bgCol = bg;
    Color txtCol = textCol;
    if (btn.pressed) {
        bgCol = pressBg;
    } else if (btn.hovered) {
        bgCol = hoverBg;
    }

    // "Load Game" button gets dimmed text (placeholder).
    if (btn.action == MenuAction::LoadGame) {
        txtCol = textDimCol;
    }

    DrawRectangleRounded(btn.bounds, BUTTON_DRAW_ROUNDNESS, BUTTON_DRAW_SEGMENTS, bgCol);
    DrawRectangleRoundedLines(btn.bounds, BUTTON_DRAW_ROUNDNESS, BUTTON_DRAW_SEGMENTS, borderCol);

    int textW = MeasureText(btn.label.c_str(), fontSize);
    auto boundsX = static_cast<int>(btn.bounds.x);
    auto boundsW = static_cast<int>(btn.bounds.width);
    auto boundsY = static_cast<int>(btn.bounds.y);
    auto boundsH = static_cast<int>(btn.bounds.height);
    int textX = boundsX + ((boundsW - textW) / 2);
    int textY = boundsY + ((boundsH - fontSize) / 2);
    DrawText(btn.label.c_str(), textX, textY, fontSize, txtCol);
}

// ═══════════════════════════════════════════════════════════════════════════════
// MainMenu
// ═══════════════════════════════════════════════════════════════════════════════

MainMenu::MainMenu() {
    buttons_.push_back({"New Game", MenuAction::NewGame, {}, false, false});
    buttons_.push_back({"Load Game", MenuAction::LoadGame, {}, false, false});
    buttons_.push_back({"Settings", MenuAction::Settings, {}, false, false});
    buttons_.push_back({"Quit", MenuAction::Quit, {}, false, false});
}

void MainMenu::layoutButtons(int screenWidth, int screenHeight) {
    auto numButtons = static_cast<int>(buttons_.size());
    int totalH = (numButtons * BUTTON_HEIGHT) + ((numButtons - 1) * BUTTON_SPACING);
    int startY = (screenHeight / 2) - (totalH / 2);
    int startX = (screenWidth / 2) - (BUTTON_WIDTH / 2);

    for (std::size_t i = 0; i < buttons_.size(); ++i) {
        auto idx = static_cast<int>(i);
        buttons_[i].bounds = {.x = static_cast<float>(startX),
                              .y = static_cast<float>(startY + (idx * (BUTTON_HEIGHT + BUTTON_SPACING))),
                              .width = static_cast<float>(BUTTON_WIDTH),
                              .height = static_cast<float>(BUTTON_HEIGHT)};
    }
}

MenuAction MainMenu::update(int screenWidth, int screenHeight) {
    layoutButtons(screenWidth, screenHeight);

    for (auto &btn : buttons_) {
        btn.hovered = isMouseOver(btn.bounds);
        btn.pressed = btn.hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

        if (btn.hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            return btn.action;
        }
    }
    return MenuAction::None;
}

void MainMenu::draw(int screenWidth, int screenHeight) const {
    DrawRectangle(0, 0, screenWidth, screenHeight, BG_COLOR);

    // Title.
    const char *title = "My4X";
    int titleW = MeasureText(title, TITLE_FONT_SIZE);
    DrawText(title, (screenWidth / 2) - (titleW / 2), (screenHeight / 2) - TITLE_Y_OFFSET, TITLE_FONT_SIZE,
             TITLE_COLOR);

    // Subtitle.
    const char *subtitle = "A 4X Strategy Game";
    int subtitleW = MeasureText(subtitle, SUBTITLE_FONT_SIZE);
    DrawText(subtitle, (screenWidth / 2) - (subtitleW / 2), ((screenHeight / 2) - TITLE_Y_OFFSET) + SUBTITLE_Y_OFFSET,
             SUBTITLE_FONT_SIZE, SUBTITLE_COLOR);

    // Buttons.
    for (const auto &btn : buttons_) {
        drawStyledButton(btn, BUTTON_FONT_SIZE, BUTTON_BG, BUTTON_HOVER_BG, BUTTON_PRESS_BG, BUTTON_BORDER, BUTTON_TEXT,
                         BUTTON_TEXT_DIM);
    }

    // Version text.
    const char *version = "v0.1.0";
    int versionW = MeasureText(version, VERSION_FONT_SIZE);
    DrawText(version, (screenWidth / 2) - (versionW / 2), screenHeight - VERSION_MARGIN - VERSION_FONT_SIZE,
             VERSION_FONT_SIZE, VERSION_COLOR);
}

// ═══════════════════════════════════════════════════════════════════════════════
// SettingsScreen
// ═══════════════════════════════════════════════════════════════════════════════

SettingsScreen::SettingsScreen()
    : backButton_{.label = "Back", .action = MenuAction::BackToMenu, .bounds = {}, .hovered = false, .pressed = false} {
}

void SettingsScreen::layoutButtons(int screenWidth, int screenHeight) {
    backButton_.bounds = {.x = (static_cast<float>(screenWidth) / HALF) - (static_cast<float>(BACK_BUTTON_W) / HALF),
                          .y = static_cast<float>(screenHeight - BACK_BUTTON_MARGIN - BACK_BUTTON_H),
                          .width = static_cast<float>(BACK_BUTTON_W),
                          .height = static_cast<float>(BACK_BUTTON_H)};
}

MenuAction SettingsScreen::update(int screenWidth, int screenHeight) {
    layoutButtons(screenWidth, screenHeight);

    backButton_.hovered = isMouseOver(backButton_.bounds);
    backButton_.pressed = backButton_.hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    if (backButton_.hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        return backButton_.action;
    }

    // ESC also goes back.
    if (IsKeyPressed(KEY_ESCAPE)) {
        return MenuAction::BackToMenu;
    }

    return MenuAction::None;
}

void SettingsScreen::draw(int screenWidth, int screenHeight) const {
    DrawRectangle(0, 0, screenWidth, screenHeight, BG_COLOR);

    // Heading.
    const char *heading = "Settings";
    int headingW = MeasureText(heading, HEADING_FONT_SIZE);
    DrawText(heading, (screenWidth / 2) - (headingW / 2), HEADING_Y_OFFSET, HEADING_FONT_SIZE, HEADING_COLOR);

    // Settings rows (placeholder values).
    int rowY = SETTINGS_Y_START;
    int labelX = (screenWidth / 2) - LABEL_X_OFFSET;
    int valueX = (screenWidth / 2) + VALUE_X_OFFSET;

    DrawText("Resolution", labelX, rowY, LABEL_FONT_SIZE, LABEL_COLOR);
    std::string resStr = std::to_string(GetScreenWidth()) + " x " + std::to_string(GetScreenHeight());
    DrawText(resStr.c_str(), valueX, rowY, VALUE_FONT_SIZE, VALUE_COLOR);
    rowY += ROW_SPACING;

    DrawText("Fullscreen", labelX, rowY, LABEL_FONT_SIZE, LABEL_COLOR);
    DrawText("Off", valueX, rowY, VALUE_FONT_SIZE, VALUE_COLOR);
    rowY += ROW_SPACING;

    DrawText("Master Volume", labelX, rowY, LABEL_FONT_SIZE, LABEL_COLOR);
    DrawText("100%", valueX, rowY, VALUE_FONT_SIZE, VALUE_COLOR);
    rowY += ROW_SPACING;

    DrawText("Music Volume", labelX, rowY, LABEL_FONT_SIZE, LABEL_COLOR);
    DrawText("80%", valueX, rowY, VALUE_FONT_SIZE, VALUE_COLOR);
    rowY += ROW_SPACING;

    DrawText("SFX Volume", labelX, rowY, LABEL_FONT_SIZE, LABEL_COLOR);
    DrawText("100%", valueX, rowY, VALUE_FONT_SIZE, VALUE_COLOR);

    // Note about placeholders.
    const char *note = "(Settings are placeholders for future implementation)";
    int noteW = MeasureText(note, LABEL_FONT_SIZE);
    DrawText(note, (screenWidth / 2) - (noteW / 2), rowY + ROW_SPACING + (ROW_SPACING / 2), LABEL_FONT_SIZE, DIM_COLOR);

    // Back button.
    drawStyledButton(backButton_, LABEL_FONT_SIZE, BUTTON_BG, BUTTON_HOVER_BG, BUTTON_PRESS_BG, BUTTON_BORDER,
                     BUTTON_TEXT, BUTTON_TEXT);
}

} // namespace engine
