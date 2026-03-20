// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
#include "engine/FactionColors.h"

#include <gtest/gtest.h>

using engine::faction_colors::CITY_FILL_ALPHA;
using engine::faction_colors::CITY_FILL_SELECTED_ALPHA;
using engine::faction_colors::cityTerritoryColor;
using engine::faction_colors::factionColor;
using engine::faction_colors::factionWireColor;
using engine::faction_colors::NEUTRAL_HOSTILE_COLOR;
using engine::faction_colors::NEUTRAL_HOSTILE_WIRE;
using engine::faction_colors::NEUTRAL_PASSIVE_COLOR;
using engine::faction_colors::NEUTRAL_PASSIVE_WIRE;
using engine::faction_colors::PLAYER_PALETTE;
using engine::faction_colors::PLAYER_PALETTE_SIZE;
using engine::faction_colors::PLAYER_PALETTE_WIRE;

// ── Palette size ─────────────────────────────────────────────────────────────

TEST(FactionColorsTest, PaletteHasAtLeast8Colors) {
    EXPECT_GE(PLAYER_PALETTE_SIZE, 8U);
    EXPECT_EQ(PLAYER_PALETTE.size(), PLAYER_PALETTE_SIZE);
    EXPECT_EQ(PLAYER_PALETTE_WIRE.size(), PLAYER_PALETTE_SIZE);
}

// ── Player palette colors are fully opaque ───────────────────────────────────

TEST(FactionColorsTest, PlayerPaletteColorsAreFullyOpaque) {
    for (std::size_t i = 0; i < PLAYER_PALETTE_SIZE; ++i) {
        EXPECT_EQ(PLAYER_PALETTE[i].a, 255) << "Palette index " << i;
        EXPECT_EQ(PLAYER_PALETTE_WIRE[i].a, 255) << "Wire palette index " << i;
    }
}

// ── All 8 palette colors are distinct ────────────────────────────────────────

TEST(FactionColorsTest, AllPaletteColorsAreDistinct) {
    for (std::size_t i = 0; i < PLAYER_PALETTE_SIZE; ++i) {
        for (std::size_t j = i + 1; j < PLAYER_PALETTE_SIZE; ++j) {
            const auto &a = PLAYER_PALETTE[i];
            const auto &b = PLAYER_PALETTE[j];
            bool same = (a.r == b.r) && (a.g == b.g) && (a.b == b.b);
            EXPECT_FALSE(same) << "Palette colors " << i << " and " << j << " are identical";
        }
    }
}

// ── factionColor returns palette entry for Player type ───────────────────────

TEST(FactionColorsTest, FactionColorReturnsPlayerPaletteEntry) {
    for (std::uint8_t i = 0; i < PLAYER_PALETTE_SIZE; ++i) {
        Color c = factionColor(game::FactionType::Player, i);
        EXPECT_EQ(c.r, PLAYER_PALETTE[i].r) << "Index " << static_cast<int>(i);
        EXPECT_EQ(c.g, PLAYER_PALETTE[i].g) << "Index " << static_cast<int>(i);
        EXPECT_EQ(c.b, PLAYER_PALETTE[i].b) << "Index " << static_cast<int>(i);
        EXPECT_EQ(c.a, PLAYER_PALETTE[i].a) << "Index " << static_cast<int>(i);
    }
}

// ── factionColor wraps around for indices >= palette size ────────────────────

TEST(FactionColorsTest, FactionColorWrapsAroundForLargeIndex) {
    Color c = factionColor(game::FactionType::Player, static_cast<std::uint8_t>(PLAYER_PALETTE_SIZE));
    EXPECT_EQ(c.r, PLAYER_PALETTE[0].r);
    EXPECT_EQ(c.g, PLAYER_PALETTE[0].g);
    EXPECT_EQ(c.b, PLAYER_PALETTE[0].b);
}

// ── factionWireColor returns wire palette for Player type ────────────────────

TEST(FactionColorsTest, FactionWireColorReturnsWirePaletteEntry) {
    for (std::uint8_t i = 0; i < PLAYER_PALETTE_SIZE; ++i) {
        Color c = factionWireColor(game::FactionType::Player, i);
        EXPECT_EQ(c.r, PLAYER_PALETTE_WIRE[i].r) << "Index " << static_cast<int>(i);
        EXPECT_EQ(c.g, PLAYER_PALETTE_WIRE[i].g) << "Index " << static_cast<int>(i);
        EXPECT_EQ(c.b, PLAYER_PALETTE_WIRE[i].b) << "Index " << static_cast<int>(i);
    }
}

// ── Neutral passive returns dedicated colors ─────────────────────────────────

TEST(FactionColorsTest, NeutralPassiveColorScheme) {
    Color fill = factionColor(game::FactionType::NeutralPassive, 0);
    EXPECT_EQ(fill.r, NEUTRAL_PASSIVE_COLOR.r);
    EXPECT_EQ(fill.g, NEUTRAL_PASSIVE_COLOR.g);
    EXPECT_EQ(fill.b, NEUTRAL_PASSIVE_COLOR.b);

    Color wire = factionWireColor(game::FactionType::NeutralPassive, 0);
    EXPECT_EQ(wire.r, NEUTRAL_PASSIVE_WIRE.r);
    EXPECT_EQ(wire.g, NEUTRAL_PASSIVE_WIRE.g);
    EXPECT_EQ(wire.b, NEUTRAL_PASSIVE_WIRE.b);
}

// ── Neutral hostile returns dedicated colors ─────────────────────────────────

TEST(FactionColorsTest, NeutralHostileColorScheme) {
    Color fill = factionColor(game::FactionType::NeutralHostile, 0);
    EXPECT_EQ(fill.r, NEUTRAL_HOSTILE_COLOR.r);
    EXPECT_EQ(fill.g, NEUTRAL_HOSTILE_COLOR.g);
    EXPECT_EQ(fill.b, NEUTRAL_HOSTILE_COLOR.b);

    Color wire = factionWireColor(game::FactionType::NeutralHostile, 0);
    EXPECT_EQ(wire.r, NEUTRAL_HOSTILE_WIRE.r);
    EXPECT_EQ(wire.g, NEUTRAL_HOSTILE_WIRE.g);
    EXPECT_EQ(wire.b, NEUTRAL_HOSTILE_WIRE.b);
}

// ── Neutral ignores color index ──────────────────────────────────────────────

TEST(FactionColorsTest, NeutralIgnoresColorIndex) {
    Color c0 = factionColor(game::FactionType::NeutralPassive, 0);
    Color c5 = factionColor(game::FactionType::NeutralPassive, 5);
    EXPECT_EQ(c0.r, c5.r);
    EXPECT_EQ(c0.g, c5.g);
    EXPECT_EQ(c0.b, c5.b);

    Color h0 = factionColor(game::FactionType::NeutralHostile, 0);
    Color h3 = factionColor(game::FactionType::NeutralHostile, 3);
    EXPECT_EQ(h0.r, h3.r);
    EXPECT_EQ(h0.g, h3.g);
    EXPECT_EQ(h0.b, h3.b);
}

// ── cityTerritoryColor uses correct alpha ────────────────────────────────────

TEST(FactionColorsTest, CityTerritoryColorUnselectedAlpha) {
    Color c = cityTerritoryColor(game::FactionType::Player, 0, false);
    EXPECT_EQ(c.a, CITY_FILL_ALPHA);
    // RGB should match the palette entry.
    EXPECT_EQ(c.r, PLAYER_PALETTE[0].r);
    EXPECT_EQ(c.g, PLAYER_PALETTE[0].g);
    EXPECT_EQ(c.b, PLAYER_PALETTE[0].b);
}

TEST(FactionColorsTest, CityTerritoryColorSelectedAlpha) {
    Color c = cityTerritoryColor(game::FactionType::Player, 0, true);
    EXPECT_EQ(c.a, CITY_FILL_SELECTED_ALPHA);
}

// ── Neutral passive territory color uses gray tones ──────────────────────────

TEST(FactionColorsTest, NeutralPassiveCityTerritoryColor) {
    Color c = cityTerritoryColor(game::FactionType::NeutralPassive, 0, false);
    EXPECT_EQ(c.r, NEUTRAL_PASSIVE_COLOR.r);
    EXPECT_EQ(c.g, NEUTRAL_PASSIVE_COLOR.g);
    EXPECT_EQ(c.b, NEUTRAL_PASSIVE_COLOR.b);
    EXPECT_EQ(c.a, CITY_FILL_ALPHA);
}

// ── Wire colors are brighter than fill colors ────────────────────────────────

TEST(FactionColorsTest, WireColorsBrighterThanFillColors) {
    for (std::size_t i = 0; i < PLAYER_PALETTE_SIZE; ++i) {
        int fillSum = PLAYER_PALETTE[i].r + PLAYER_PALETTE[i].g + PLAYER_PALETTE[i].b;
        int wireSum = PLAYER_PALETTE_WIRE[i].r + PLAYER_PALETTE_WIRE[i].g + PLAYER_PALETTE_WIRE[i].b;
        EXPECT_GT(wireSum, fillSum) << "Wire should be brighter than fill at index " << i;
    }
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
