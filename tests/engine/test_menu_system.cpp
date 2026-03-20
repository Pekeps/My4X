// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "engine/MenuSystem.h"

#include <gtest/gtest.h>

// Tests for menu system types and basic construction.
// Full interaction tests require raylib window, so we test what we can.

TEST(MenuSystemTest, ScreenEnumValues) {
    // Verify all screen values are distinct.
    EXPECT_NE(static_cast<int>(engine::Screen::MainMenu), static_cast<int>(engine::Screen::InGame));
    EXPECT_NE(static_cast<int>(engine::Screen::MainMenu), static_cast<int>(engine::Screen::Settings));
    EXPECT_NE(static_cast<int>(engine::Screen::Settings), static_cast<int>(engine::Screen::InGame));
}

TEST(MenuSystemTest, MenuActionEnumValues) {
    // Verify all action values are distinct.
    EXPECT_NE(static_cast<int>(engine::MenuAction::None), static_cast<int>(engine::MenuAction::NewGame));
    EXPECT_NE(static_cast<int>(engine::MenuAction::NewGame), static_cast<int>(engine::MenuAction::LoadGame));
    EXPECT_NE(static_cast<int>(engine::MenuAction::LoadGame), static_cast<int>(engine::MenuAction::Settings));
    EXPECT_NE(static_cast<int>(engine::MenuAction::Settings), static_cast<int>(engine::MenuAction::Quit));
    EXPECT_NE(static_cast<int>(engine::MenuAction::Quit), static_cast<int>(engine::MenuAction::BackToMenu));
}

TEST(MenuSystemTest, MainMenuConstructs) {
    // Just verify it doesn't crash.
    engine::MainMenu menu;
    (void)menu;
}

TEST(MenuSystemTest, SettingsScreenConstructs) {
    // Just verify it doesn't crash.
    engine::SettingsScreen settings;
    (void)settings;
}

TEST(MenuButtonTest, DefaultState) {
    engine::MenuButton btn{"Test", engine::MenuAction::None, {0, 0, 100, 50}, false, false};
    EXPECT_EQ(btn.label, "Test");
    EXPECT_EQ(btn.action, engine::MenuAction::None);
    EXPECT_FALSE(btn.hovered);
    EXPECT_FALSE(btn.pressed);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
