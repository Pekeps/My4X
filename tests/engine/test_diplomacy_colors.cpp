// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
#include "engine/DiplomacyColors.h"

#include <gtest/gtest.h>

using engine::diplomacy_colors::ALLIANCE_COLOR;
using engine::diplomacy_colors::diplomacyColor;
using engine::diplomacy_colors::diplomacyLabel;
using engine::diplomacy_colors::PEACE_COLOR;
using engine::diplomacy_colors::WAR_COLOR;

// ── diplomacyColor returns correct colors ────────────────────────────────────

TEST(DiplomacyColorsTest, WarReturnsRedColor) {
    Color c = diplomacyColor(game::DiplomacyState::War);
    EXPECT_EQ(c.r, WAR_COLOR.r);
    EXPECT_EQ(c.g, WAR_COLOR.g);
    EXPECT_EQ(c.b, WAR_COLOR.b);
    EXPECT_EQ(c.a, WAR_COLOR.a);
}

TEST(DiplomacyColorsTest, AllianceReturnsGreenColor) {
    Color c = diplomacyColor(game::DiplomacyState::Alliance);
    EXPECT_EQ(c.r, ALLIANCE_COLOR.r);
    EXPECT_EQ(c.g, ALLIANCE_COLOR.g);
    EXPECT_EQ(c.b, ALLIANCE_COLOR.b);
    EXPECT_EQ(c.a, ALLIANCE_COLOR.a);
}

TEST(DiplomacyColorsTest, PeaceReturnsGrayColor) {
    Color c = diplomacyColor(game::DiplomacyState::Peace);
    EXPECT_EQ(c.r, PEACE_COLOR.r);
    EXPECT_EQ(c.g, PEACE_COLOR.g);
    EXPECT_EQ(c.b, PEACE_COLOR.b);
    EXPECT_EQ(c.a, PEACE_COLOR.a);
}

// ── All diplomacy colors are fully opaque ────────────────────────────────────

TEST(DiplomacyColorsTest, AllColorsAreFullyOpaque) {
    EXPECT_EQ(WAR_COLOR.a, 255);
    EXPECT_EQ(ALLIANCE_COLOR.a, 255);
    EXPECT_EQ(PEACE_COLOR.a, 255);
}

// ── All three diplomacy colors are visually distinct ─────────────────────────

TEST(DiplomacyColorsTest, AllThreeColorsAreDistinct) {
    auto colorsEqual = [](Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; };
    EXPECT_FALSE(colorsEqual(WAR_COLOR, ALLIANCE_COLOR));
    EXPECT_FALSE(colorsEqual(WAR_COLOR, PEACE_COLOR));
    EXPECT_FALSE(colorsEqual(ALLIANCE_COLOR, PEACE_COLOR));
}

// ── diplomacyLabel returns non-empty strings ─────────────────────────────────

TEST(DiplomacyColorsTest, WarLabelIsNonEmpty) {
    const char *label = diplomacyLabel(game::DiplomacyState::War);
    EXPECT_NE(label, nullptr);
    EXPECT_GT(strlen(label), 0U);
}

TEST(DiplomacyColorsTest, AllianceLabelIsNonEmpty) {
    const char *label = diplomacyLabel(game::DiplomacyState::Alliance);
    EXPECT_NE(label, nullptr);
    EXPECT_GT(strlen(label), 0U);
}

TEST(DiplomacyColorsTest, PeaceLabelIsNonEmpty) {
    const char *label = diplomacyLabel(game::DiplomacyState::Peace);
    EXPECT_NE(label, nullptr);
    EXPECT_GT(strlen(label), 0U);
}

// ── Each state maps to a different label ────────────────────────────────────

TEST(DiplomacyColorsTest, EachStateHasDistinctLabel) {
    const char *warLabel = diplomacyLabel(game::DiplomacyState::War);
    const char *allianceLabel = diplomacyLabel(game::DiplomacyState::Alliance);
    const char *peaceLabel = diplomacyLabel(game::DiplomacyState::Peace);
    EXPECT_STRNE(warLabel, allianceLabel);
    EXPECT_STRNE(warLabel, peaceLabel);
    EXPECT_STRNE(allianceLabel, peaceLabel);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
