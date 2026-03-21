// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-pro-type-vararg)

#include "engine/BuildingRenderer.h"
#include "engine/Camera.h"
#include "engine/CameraCinematic.h"
#include "engine/CityRenderer.h"
#include "engine/CombatEffects.h"
#include "engine/CombatLogRenderer.h"
#include "engine/DiplomacyColors.h"
#include "engine/EndTurnButton.h"
#include "engine/FactionColors.h"
#include "engine/HexGrid.h"
#include "engine/Input.h"
#include "engine/MapRenderer.h"
#include "engine/MenuSystem.h"
#include "engine/Minimap.h"
#include "engine/ModelManager.h"
#include "engine/RangeOverlay.h"
#include "engine/Tooltip.h"
#include "engine/Transition.h"
#include "engine/UnitAnimator.h"
#include "engine/UnitRenderer.h"
#include "engine/Window.h"
#include "game/AttackAction.h"
#include "game/BuildQueue.h"
#include "game/Building.h"
#include "game/CaptureAction.h"
#include "game/City.h"
#include "game/CombatLog.h"
#include "game/DiplomacyManager.h"
#include "game/Faction.h"
#include "game/FactionRegistry.h"
#include "game/GameState.h"
#include "game/MoveAction.h"
#include "game/NeutralAI.h"
#include "game/Pathfinding.h"
#include "game/SaveLoad.h"
#include "game/TerrainType.h"
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include "raylib.h"

#include <algorithm>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// ── Screen & map ─────────────────────────────────────────────────────────────

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int MAP_ROWS = 40;
const int MAP_COLS = 24;

// ── Debug mode ──────────────────────────────────────────────────────────────

/// Set to true to disable fog of war and reveal the entire map.
bool DEBUG_NO_FOG = false;

// ── Top-left HUD ─────────────────────────────────────────────────────────────

const int HUD_X = 14;
const int HUD_Y = 12;
const int HUD_TITLE_SIZE = 22;
const int HUD_TEXT_SIZE = 16;
const int HUD_LINE_H = 22;

// ── City panel (right side) ──────────────────────────────────────────────────

const int PANEL_W = 340;
const int PANEL_MARGIN = 12;
const int PANEL_X = SCREEN_WIDTH - PANEL_W - PANEL_MARGIN;
const int PANEL_Y = PANEL_MARGIN;
const int PANEL_PAD = 14;
const int PANEL_TITLE_SIZE = 22;
const int PANEL_HEADING_SIZE = 14;
const int PANEL_TEXT_SIZE = 16;
const int PANEL_KEY_SIZE = 14;
const int PANEL_LINE_H = 22;
const int PANEL_SECTION_GAP = 10;
const int PROGRESS_BAR_H = 14;

const Color PANEL_BG = {16, 18, 26, 230};
const Color PANEL_BORDER = {60, 65, 90, 255};
const Color PANEL_TITLE_COL = {220, 200, 120, 255};
const Color PANEL_HEADING_COL = {130, 140, 170, 255};
const Color PANEL_TEXT_COL = {200, 200, 210, 255};
const Color PANEL_DIM_COL = {120, 120, 140, 255};
const Color PANEL_KEY_COL = {100, 180, 255, 255};
const Color PANEL_KEY_BG = {40, 50, 70, 255};
const Color PROGRESS_BG = {30, 32, 44, 255};
const Color PROGRESS_FILL = {70, 180, 100, 255};
const Color SEPARATOR_COL = {50, 55, 75, 255};

const Color HUD_BG = {16, 18, 26, 200};
const Color HUD_TITLE_COL = {255, 255, 255, 255};
const Color HUD_TEXT_COL = {180, 180, 190, 255};
const Color HUD_ACCENT_COL = {220, 200, 120, 255};
const Color HUD_DIM_COL = {100, 100, 120, 255};

const int NO_SELECTION = -1;

// ── Unit info panel (left side, when unit selected) ──────────────────────────

const int UNIT_PANEL_X = 14;
const int UNIT_PANEL_Y = 12;
const int UNIT_PANEL_W = 340;

// ── Faction list panel (bottom-left) ─────────────────────────────────────────

const int FACTION_PANEL_X = 14;
const int FACTION_PANEL_Y_OFFSET = 60;
const int FACTION_PANEL_W = 300;
const int FACTION_DOT_SIZE = 10;
const int FACTION_DOT_GAP = 6;

// ── Player faction ID (first registered faction) ────────────────────────────

const game::FactionId PLAYER_FACTION_ID = 1;

// ── Damage number colors ─────────────────────────────────────────────────────

const Color DAMAGE_COLOR_NORMAL = {255, 80, 80, 255};
const Color DAMAGE_COLOR_COUNTER = {255, 180, 60, 255};

// ── Damage number Y spawn offset ─────────────────────────────────────────────

const float DAMAGE_NUMBER_SPAWN_Y = 1.2F;

// ── Demo setup ───────────────────────────────────────────────────────────────

static void setupDemoState(game::GameState &state) {
    auto &registry = state.mutableFactionRegistry();
    game::UnitTypeRegistry unitReg;
    unitReg.registerDefaults();

    // ── Player faction: Roman Empire ─────────────────────────────────────
    game::FactionId playerId = registry.addFaction("Roman Empire", game::FactionType::Player, 0);
    auto &playerFaction = registry.getMutableFaction(playerId);
    playerFaction.addResources(game::Resource{.gold = 100, .production = 40, .food = 60});
    state.factionResources() += game::Resource{.gold = 100, .production = 40, .food = 60};

    // Capital city with surrounding territory.
    game::City capital("Roma", 10, 5, static_cast<int>(playerId));
    capital.addTile(10, 6);
    capital.addTile(9, 5);
    capital.addTile(11, 5);
    state.addCity(std::move(capital));
    state.addBuilding(game::makeFarm(10, 6));
    state.addBuilding(game::makeFarm(9, 5));

    // Second city — frontier outpost.
    game::City outpost("Mediolanum", 14, 8, static_cast<int>(playerId));
    outpost.addTile(14, 9);
    outpost.addTile(13, 8);
    state.addCity(std::move(outpost));
    state.addBuilding(game::makeFarm(14, 9));

    // Player units — a garrison and a field army.
    state.addUnit(std::make_unique<game::Warrior>(10, 4, unitReg, playerId));
    state.addUnit(std::make_unique<game::Warrior>(11, 6, unitReg, playerId));
    state.addUnit(std::make_unique<game::Unit>(10, 7, unitReg.getTemplate("Archer"), playerId));
    state.addUnit(std::make_unique<game::Warrior>(14, 7, unitReg, playerId));
    state.addUnit(std::make_unique<game::Unit>(13, 9, unitReg.getTemplate("Settler"), playerId));

    // ── AI faction: Persian Empire ───────────────────────────────────────
    game::FactionId persiaId = registry.addFaction("Persian Empire", game::FactionType::Player, 1);
    auto &persiaFaction = registry.getMutableFaction(persiaId);
    persiaFaction.addResources(game::Resource{.gold = 80, .production = 35, .food = 50});

    game::City persepolis("Persepolis", 8, 18, static_cast<int>(persiaId));
    persepolis.addTile(8, 19);
    persepolis.addTile(7, 18);
    state.addCity(std::move(persepolis));
    state.addBuilding(game::makeFarm(8, 19));

    game::City susa("Susa", 12, 20, static_cast<int>(persiaId));
    susa.addTile(12, 21);
    state.addCity(std::move(susa));

    state.addUnit(std::make_unique<game::Warrior>(8, 17, unitReg, persiaId));
    state.addUnit(std::make_unique<game::Warrior>(9, 19, unitReg, persiaId));
    state.addUnit(std::make_unique<game::Unit>(7, 19, unitReg.getTemplate("Archer"), persiaId));
    state.addUnit(std::make_unique<game::Warrior>(12, 19, unitReg, persiaId));

    // ── Spawn neutral factions with units and diplomacy ──────────────────
    game::NeutralAI::spawnNeutralFactions(state);

    // Initialize diplomacy between all factions.
    state.mutableDiplomacy().initializeDefaults(registry);

    // Calculate initial fog of war for the player faction.
    state.recalculateFog(playerId);
}

// ── City click detection ─────────────────────────────────────────────────────

static void handleCityClick(const std::vector<game::City> &cities, const engine::hex::TileCoord &tile,
                            std::optional<game::CityId> &selectedCity) {
    for (const auto &city : cities) {
        if (city.containsTile(tile.row, tile.col)) {
            selectedCity = city.id();
            return;
        }
    }
    selectedCity = std::nullopt;
}

// ── Panel drawing helpers ────────────────────────────────────────────────────

static void drawSeparator(int x, int y, int w) { DrawRectangle(x, y, w, 1, SEPARATOR_COL); }

static void drawKeyHint(int x, int y, const char *key, const char *label) {
    int keyW = MeasureText(key, PANEL_KEY_SIZE) + 10;
    DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y - 1), static_cast<float>(keyW),
                          static_cast<float>(PANEL_KEY_SIZE + 4)},
                         0.3F, 4, PANEL_KEY_BG);
    DrawText(key, x + 5, y, PANEL_KEY_SIZE, PANEL_KEY_COL);
    DrawText(label, x + keyW + 6, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);
}

static void drawProgressBar(int x, int y, int w, int current, int total) {
    DrawRectangle(x, y, w, PROGRESS_BAR_H, PROGRESS_BG);
    if (total > 0) {
        int fillW = std::min(w, (current * w) / total);
        DrawRectangle(x, y, fillW, PROGRESS_BAR_H, PROGRESS_FILL);
    }
    std::string text = std::to_string(current) + "/" + std::to_string(total);
    int textW = MeasureText(text.c_str(), PANEL_KEY_SIZE);
    DrawText(text.c_str(), x + ((w - textW) / 2), y + 1, PANEL_KEY_SIZE, WHITE);
}

// ── Unit info panel (left side) ─────────────────────────────────────────────

static void drawUnitInfoPanel(const game::GameState &state, int selectedUnit) {
    if (selectedUnit == NO_SELECTION) {
        return;
    }

    const auto &unit = state.units().at(selectedUnit);
    const auto *playerFaction = state.factionRegistry().findFaction(PLAYER_FACTION_ID);

    int contentW = UNIT_PANEL_W - (PANEL_PAD * 2);
    int x = UNIT_PANEL_X + PANEL_PAD;

    // Measure panel height dynamically.
    int panelH = PANEL_PAD;          // top padding
    panelH += PANEL_TITLE_SIZE + 8;  // unit name
    panelH += 1 + PANEL_SECTION_GAP; // separator
    if (playerFaction != nullptr) {
        panelH += PANEL_LINE_H; // faction name line
    }
    panelH += PANEL_LINE_H;          // resources
    panelH += 1 + PANEL_SECTION_GAP; // separator
    panelH += PANEL_LINE_H * 4;      // HP, ATK/DEF, Moves, XP/Level
    panelH += PANEL_SECTION_GAP;
    panelH += 1 + PANEL_SECTION_GAP; // separator

    // Terrain line
    panelH += PANEL_LINE_H;

    // Faction/diplomacy for the selected unit.
    const auto *unitFaction = state.factionRegistry().findFaction(unit->factionId());
    if (unitFaction != nullptr) {
        panelH += PANEL_LINE_H;
    }

    panelH += PANEL_SECTION_GAP;
    panelH += PANEL_LINE_H; // controls hint
    panelH += PANEL_PAD;    // bottom padding

    // Panel background with border.
    DrawRectangleRounded({static_cast<float>(UNIT_PANEL_X), static_cast<float>(UNIT_PANEL_Y),
                          static_cast<float>(UNIT_PANEL_W), static_cast<float>(panelH)},
                         0.04F, 6, PANEL_BG);
    DrawRectangleRoundedLines({static_cast<float>(UNIT_PANEL_X), static_cast<float>(UNIT_PANEL_Y),
                               static_cast<float>(UNIT_PANEL_W), static_cast<float>(panelH)},
                              0.04F, 6, PANEL_BORDER);

    int y = UNIT_PANEL_Y + PANEL_PAD;

    // ── Unit name ───────────────────────────────────────────────────────
    DrawText(unit->name().c_str(), x, y, PANEL_TITLE_SIZE, PANEL_TITLE_COL);
    y += PANEL_TITLE_SIZE + 8;

    drawSeparator(x, y, contentW);
    y += 1 + PANEL_SECTION_GAP;

    // ── Player faction + resources ──────────────────────────────────────
    if (playerFaction != nullptr) {
        std::string factionLabel = "Faction: " + playerFaction->name();
        Color factionColor = engine::faction_colors::factionColor(playerFaction->type(), playerFaction->colorIndex());
        DrawText(factionLabel.c_str(), x, y, HUD_TEXT_SIZE, factionColor);
        y += HUD_LINE_H;
    }

    // Resources.
    const game::Resource &res = (playerFaction != nullptr) ? playerFaction->stockpile() : state.factionResources();
    std::string goldStr = "Gold: " + std::to_string(res.gold);
    std::string prodStr = "Prod: " + std::to_string(res.production);
    std::string foodStr = "Food: " + std::to_string(res.food);
    DrawText(goldStr.c_str(), x, y, HUD_TEXT_SIZE, HUD_ACCENT_COL);
    DrawText(prodStr.c_str(), x + 110, y, HUD_TEXT_SIZE, HUD_TEXT_COL);
    DrawText(foodStr.c_str(), x + 220, y, HUD_TEXT_SIZE, Color{120, 200, 100, 255});
    y += HUD_LINE_H;

    drawSeparator(x, y, contentW);
    y += 1 + PANEL_SECTION_GAP;

    // ── Unit stats ──────────────────────────────────────────────────────
    std::string hpStr = "HP    " + std::to_string(unit->health()) + " / " + std::to_string(unit->maxHealth());
    DrawText(hpStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);

    // HP bar (small inline bar).
    int hpBarX = x + contentW - 100;
    int hpBarW = 100;
    int hpBarH = 10;
    int hpBarY = y + 4;
    DrawRectangle(hpBarX, hpBarY, hpBarW, hpBarH, PROGRESS_BG);
    if (unit->maxHealth() > 0) {
        int fillW = std::min(hpBarW, (unit->health() * hpBarW) / unit->maxHealth());
        Color hpColor = {.r = 70, .g = 200, .b = 90, .a = 255};
        if (unit->health() * 3 < unit->maxHealth()) {
            hpColor = {.r = 220, .g = 60, .b = 60, .a = 255}; // low HP = red
        } else if (unit->health() * 3 < unit->maxHealth() * 2) {
            hpColor = {.r = 220, .g = 180, .b = 60, .a = 255}; // medium HP = yellow
        }
        DrawRectangle(hpBarX, hpBarY, fillW, hpBarH, hpColor);
    }
    y += PANEL_LINE_H;

    std::string atkDefStr = "ATK " + std::to_string(unit->attack()) + "    DEF " + std::to_string(unit->defense());
    if (unit->attackRange() > 1) {
        atkDefStr += "    Range " + std::to_string(unit->attackRange());
    }
    DrawText(atkDefStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);
    y += PANEL_LINE_H;

    std::string moveStr =
        "Moves " + std::to_string(unit->movementRemaining()) + " / " + std::to_string(unit->movement());
    DrawText(moveStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);
    y += PANEL_LINE_H;

    std::string xpStr = "XP " + std::to_string(unit->experience());
    if (unit->level() > 0) {
        xpStr += "    Level " + std::to_string(unit->level());
    }
    DrawText(xpStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);
    y += PANEL_LINE_H;

    y += PANEL_SECTION_GAP;
    drawSeparator(x, y, contentW);
    y += 1 + PANEL_SECTION_GAP;

    // ── Terrain info for unit's tile ────────────────────────────────────
    auto terrain = state.map().tile(unit->row(), unit->col()).terrainType();
    const auto &terrainProps = game::getTerrainProperties(terrain);
    std::string terrainStr = "Terrain: " + std::string(game::terrainName(terrain));
    if (terrainProps.defenseModifier > 0) {
        terrainStr += "  (Defense +" + std::to_string(terrainProps.defenseModifier) + ")";
    } else if (terrainProps.defenseModifier < 0) {
        terrainStr += "  (Defense " + std::to_string(terrainProps.defenseModifier) + ")";
    }
    Color terrainColor = PANEL_DIM_COL;
    if (terrainProps.defenseModifier > 0) {
        terrainColor = Color{100, 200, 255, 255};
    } else if (terrainProps.defenseModifier < 0) {
        terrainColor = Color{255, 140, 100, 255};
    }
    DrawText(terrainStr.c_str(), x, y, HUD_TEXT_SIZE, terrainColor);
    y += PANEL_LINE_H;

    // ── Unit owner faction + diplomacy ──────────────────────────────────
    if (unitFaction != nullptr) {
        Color unitFactionColor = engine::faction_colors::factionColor(unitFaction->type(), unitFaction->colorIndex());
        std::string ownerStr = unitFaction->name();
        DrawText(ownerStr.c_str(), x, y, HUD_TEXT_SIZE, unitFactionColor);

        if (unit->factionId() != PLAYER_FACTION_ID) {
            auto relation = state.diplomacy().getRelation(PLAYER_FACTION_ID, unit->factionId());
            Color dipColor = engine::diplomacy_colors::diplomacyColor(relation);
            const char *dipLabel = engine::diplomacy_colors::diplomacyLabel(relation);
            std::string dipStr = std::string("  [") + dipLabel + "]";
            int nameW = MeasureText(ownerStr.c_str(), HUD_TEXT_SIZE);
            DrawText(dipStr.c_str(), x + nameW, y, HUD_TEXT_SIZE, dipColor);
        }
        y += PANEL_LINE_H;
    }

    y += PANEL_SECTION_GAP;

    // ── Controls hint ───────────────────────────────────────────────────
    DrawText("RMB / ESC to deselect", x, y, PANEL_KEY_SIZE, HUD_DIM_COL);
}

// ── Top-left HUD (when no unit selected — shows faction info + resources) ────

static void drawTopHud(const game::GameState &state) {
    const auto *playerFaction = state.factionRegistry().findFaction(PLAYER_FACTION_ID);

    // Background — compute height dynamically.
    int hudH = 56;
    if (playerFaction != nullptr) {
        hudH += HUD_LINE_H; // faction name line
    }
    DrawRectangleRounded(
        {static_cast<float>(HUD_X - 6), static_cast<float>(HUD_Y - 4), 420.0F, static_cast<float>(hudH)}, 0.08F, 4,
        HUD_BG);

    int y = HUD_Y;

    // Player faction name.
    if (playerFaction != nullptr) {
        std::string factionLabel = "Faction: " + playerFaction->name();
        Color factionColor = engine::faction_colors::factionColor(playerFaction->type(), playerFaction->colorIndex());
        DrawText(factionLabel.c_str(), HUD_X, y, HUD_TEXT_SIZE, factionColor);
        y += HUD_LINE_H;
    }

    // Resources.
    const game::Resource &res = (playerFaction != nullptr) ? playerFaction->stockpile() : state.factionResources();
    std::string goldStr = "Gold: " + std::to_string(res.gold);
    std::string prodStr = "Prod: " + std::to_string(res.production);
    std::string foodStr = "Food: " + std::to_string(res.food);
    DrawText(goldStr.c_str(), HUD_X, y, HUD_TEXT_SIZE, HUD_ACCENT_COL);
    DrawText(prodStr.c_str(), HUD_X + 110, y, HUD_TEXT_SIZE, HUD_TEXT_COL);
    DrawText(foodStr.c_str(), HUD_X + 220, y, HUD_TEXT_SIZE, Color{120, 200, 100, 255});
    y += HUD_LINE_H;

    // Controls hint.
    DrawText("LMB select  |  RMB deselect  |  SPACE / End Turn button", HUD_X, y, PANEL_KEY_SIZE, HUD_DIM_COL);
}

// ── Faction list panel (bottom-left) ─────────────────────────────────────────

static void drawFactionListPanel(const game::GameState &state) {
    const auto &allFactions = state.factionRegistry().allFactions();
    if (allFactions.empty()) {
        return;
    }

    int numFactions = static_cast<int>(allFactions.size());
    int panelH = (PANEL_PAD * 2) + PANEL_HEADING_SIZE + 6 + (numFactions * HUD_LINE_H);
    int panelY = SCREEN_HEIGHT - FACTION_PANEL_Y_OFFSET - panelH;

    DrawRectangleRounded({static_cast<float>(FACTION_PANEL_X - 6), static_cast<float>(panelY - 4),
                          static_cast<float>(FACTION_PANEL_W), static_cast<float>(panelH)},
                         0.08F, 4, HUD_BG);

    int x = FACTION_PANEL_X;
    int y = panelY + PANEL_PAD;

    DrawText("FACTIONS", x, y, PANEL_HEADING_SIZE, PANEL_HEADING_COL);
    y += PANEL_HEADING_SIZE + 6;

    for (const auto &faction : allFactions) {
        // Draw colored dot.
        Color dotColor = engine::faction_colors::factionColor(faction.type(), faction.colorIndex());
        DrawRectangle(x, y + 4, FACTION_DOT_SIZE, FACTION_DOT_SIZE, dotColor);

        // Draw faction name.
        int textX = x + FACTION_DOT_SIZE + FACTION_DOT_GAP;
        DrawText(faction.name().c_str(), textX, y, HUD_TEXT_SIZE, PANEL_TEXT_COL);

        // Draw diplomacy relation to player (skip for the player's own faction).
        if (faction.id() != PLAYER_FACTION_ID) {
            auto relation = state.diplomacy().getRelation(PLAYER_FACTION_ID, faction.id());
            Color dipColor = engine::diplomacy_colors::diplomacyColor(relation);
            const char *dipLabel = engine::diplomacy_colors::diplomacyLabel(relation);
            int nameW = MeasureText(faction.name().c_str(), HUD_TEXT_SIZE);
            DrawText(dipLabel, textX + nameW + 8, y + 2, PANEL_KEY_SIZE, dipColor);
        } else {
            int nameW = MeasureText(faction.name().c_str(), HUD_TEXT_SIZE);
            DrawText("(You)", textX + nameW + 8, y + 2, PANEL_KEY_SIZE, HUD_DIM_COL);
        }

        y += HUD_LINE_H;
    }
}

// ── City info panel ──────────────────────────────────────────────────────────

static void drawCityPanel(const game::City &city, const game::GameState &state,
                          std::optional<engine::hex::TileCoord> hoveredTile) {
    int contentW = PANEL_W - (PANEL_PAD * 2);
    int x = PANEL_X + PANEL_PAD;

    // Count garrison units in city territory.
    int garrisonCount = 0;
    for (const auto &unit : state.units()) {
        if (unit->isAlive() && city.containsTile(unit->row(), unit->col())) {
            ++garrisonCount;
        }
    }

    // Measure panel height dynamically
    int panelH = PANEL_PAD;                              // top padding
    panelH += PANEL_TITLE_SIZE + 8;                      // city name + gap
    panelH += 1 + PANEL_SECTION_GAP;                     // separator
    panelH += PANEL_LINE_H;                              // faction line
    panelH += PANEL_LINE_H * 3;                          // stats: population, territory, production
    panelH += PANEL_LINE_H;                              // garrison line
    panelH += PANEL_SECTION_GAP + 1 + PANEL_SECTION_GAP; // separator

    panelH += PANEL_HEADING_SIZE + 6; // "BUILD QUEUE" heading

    const auto &bq = city.buildQueue();
    if (bq.isEmpty()) {
        panelH += PANEL_LINE_H; // "(idle)"
    } else {
        auto items = bq.allItems();
        panelH += PANEL_LINE_H + 4 + PROGRESS_BAR_H + 4; // current item + progress bar
        if (items.size() > 1) {
            panelH += static_cast<int>((items.size() - 1)) * PANEL_LINE_H; // queued items
        }
    }

    panelH += PANEL_SECTION_GAP + 1 + PANEL_SECTION_GAP; // separator
    panelH += PANEL_HEADING_SIZE + 6;                    // "ACTIONS" heading
    panelH += PANEL_LINE_H * 4;                          // F, M, K, X
    panelH += PANEL_SECTION_GAP;
    panelH += PANEL_LINE_H; // ESC hint
    panelH += PANEL_PAD;    // bottom padding

    // Panel background
    DrawRectangleRounded({static_cast<float>(PANEL_X), static_cast<float>(PANEL_Y), static_cast<float>(PANEL_W),
                          static_cast<float>(panelH)},
                         0.04F, 6, PANEL_BG);
    DrawRectangleRoundedLines({static_cast<float>(PANEL_X), static_cast<float>(PANEL_Y), static_cast<float>(PANEL_W),
                               static_cast<float>(panelH)},
                              0.04F, 6, PANEL_BORDER);

    int y = PANEL_Y + PANEL_PAD;

    // ── City Name ────────────────────────────────────────────────────────
    DrawText(city.name().c_str(), x, y, PANEL_TITLE_SIZE, PANEL_TITLE_COL);
    y += PANEL_TITLE_SIZE + 8;

    drawSeparator(x, y, contentW);
    y += 1 + PANEL_SECTION_GAP;

    // ── Owning faction ──────────────────────────────────────────────────
    const auto *cityFaction = state.factionRegistry().findFaction(static_cast<game::FactionId>(city.factionId()));
    if (cityFaction != nullptr) {
        Color factionColor = engine::faction_colors::factionColor(cityFaction->type(), cityFaction->colorIndex());
        // Draw colored dot + faction name.
        DrawRectangle(x, y + 4, FACTION_DOT_SIZE, FACTION_DOT_SIZE, factionColor);
        std::string factionNameStr = cityFaction->name();
        DrawText(factionNameStr.c_str(), x + FACTION_DOT_SIZE + FACTION_DOT_GAP, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);

        // Show diplomacy relation if foreign city.
        auto cityFactionId = static_cast<game::FactionId>(city.factionId());
        if (cityFactionId != PLAYER_FACTION_ID) {
            auto relation = state.diplomacy().getRelation(PLAYER_FACTION_ID, cityFactionId);
            Color dipColor = engine::diplomacy_colors::diplomacyColor(relation);
            const char *dipLabel = engine::diplomacy_colors::diplomacyLabel(relation);
            int nameW = MeasureText(factionNameStr.c_str(), PANEL_TEXT_SIZE);
            DrawText(dipLabel, x + FACTION_DOT_SIZE + FACTION_DOT_GAP + nameW + 8, y + 2, PANEL_KEY_SIZE, dipColor);
        }
    } else {
        std::string factionStr = "Faction " + std::to_string(city.factionId());
        DrawText(factionStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_DIM_COL);
    }
    y += PANEL_LINE_H;

    // ── Stats ────────────────────────────────────────────────────────────
    std::string popStr = "Population    " + std::to_string(city.population());
    DrawText(popStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);
    y += PANEL_LINE_H;

    std::string terStr = "Territory     " + std::to_string(city.tiles().size()) + " tiles";
    DrawText(terStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);
    y += PANEL_LINE_H;

    std::string prodStr = "Production    " + std::to_string(game::City::productionPerTurn()) + "/turn";
    DrawText(prodStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);
    y += PANEL_LINE_H;

    // ── Garrison ────────────────────────────────────────────────────────
    std::string garrisonStr =
        "Garrison      " + std::to_string(garrisonCount) + " unit" + (garrisonCount != 1 ? "s" : "");
    DrawText(garrisonStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);
    y += PANEL_LINE_H;

    y += PANEL_SECTION_GAP;
    drawSeparator(x, y, contentW);
    y += 1 + PANEL_SECTION_GAP;

    // ── Build Queue ──────────────────────────────────────────────────────
    DrawText("BUILD QUEUE", x, y, PANEL_HEADING_SIZE, PANEL_HEADING_COL);
    y += PANEL_HEADING_SIZE + 6;

    if (bq.isEmpty()) {
        DrawText("(idle)", x, y, PANEL_TEXT_SIZE, PANEL_DIM_COL);
        y += PANEL_LINE_H;
    } else {
        auto items = bq.allItems();
        auto &current = items[0];
        std::string curStr =
            current.name + " at (" + std::to_string(current.targetRow) + "," + std::to_string(current.targetCol) + ")";
        DrawText(curStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_TEXT_COL);

        int turnsLeft = bq.turnsRemaining(game::City::productionPerTurn());
        std::string etaStr = std::to_string(turnsLeft) + "T";
        int etaW = MeasureText(etaStr.c_str(), PANEL_KEY_SIZE);
        DrawText(etaStr.c_str(), PANEL_X + PANEL_W - PANEL_PAD - etaW, y + 2, PANEL_KEY_SIZE, PANEL_DIM_COL);
        y += PANEL_LINE_H + 4;

        drawProgressBar(x, y, contentW, bq.accumulatedProduction(), current.productionCost);
        y += PROGRESS_BAR_H + 4;

        for (size_t i = 1; i < items.size(); ++i) {
            std::string qStr = "  " + items[i].name + " at (" + std::to_string(items[i].targetRow) + "," +
                               std::to_string(items[i].targetCol) + ")";
            DrawText(qStr.c_str(), x, y, PANEL_TEXT_SIZE, PANEL_DIM_COL);
            y += PANEL_LINE_H;
        }
    }

    y += PANEL_SECTION_GAP;
    drawSeparator(x, y, contentW);
    y += 1 + PANEL_SECTION_GAP;

    // ── Actions ──────────────────────────────────────────────────────────
    DrawText("ACTIONS", x, y, PANEL_HEADING_SIZE, PANEL_HEADING_COL);

    // Hovered tile indicator
    if (hoveredTile) {
        std::string tileStr =
            "Tile (" + std::to_string(hoveredTile->row) + "," + std::to_string(hoveredTile->col) + ")";
        int tileW = MeasureText(tileStr.c_str(), PANEL_KEY_SIZE);
        DrawText(tileStr.c_str(), PANEL_X + PANEL_W - PANEL_PAD - tileW, y, PANEL_KEY_SIZE, PANEL_DIM_COL);
    }
    y += PANEL_HEADING_SIZE + 6;

    drawKeyHint(x, y, "F", "Queue Farm       20 prod");
    y += PANEL_LINE_H;
    drawKeyHint(x, y, "M", "Queue Mine       10 prod  (Hills)");
    y += PANEL_LINE_H;
    drawKeyHint(x, y, "K", "Queue Market     30 prod");
    y += PANEL_LINE_H;

    if (!bq.isEmpty()) {
        drawKeyHint(x, y, "X", "Cancel current build");
    } else {
        DrawText("  X  Cancel build (nothing queued)", x, y, PANEL_KEY_SIZE, PANEL_DIM_COL);
    }
    y += PANEL_LINE_H;

    y += PANEL_SECTION_GAP;
    DrawText("  ESC / RMB to deselect city", x, y, PANEL_KEY_SIZE, PANEL_DIM_COL);
}

// ── Build queue actions ──────────────────────────────────────────────────────

static void handleBuildActions(game::GameState &state, game::CityId cityId,
                               std::optional<engine::hex::TileCoord> hoveredTile) {
    game::City *city = state.findCity(cityId);
    if (city == nullptr) {
        return;
    }

    // Cancel current build
    if (IsKeyPressed(KEY_X) && !city->buildQueue().isEmpty()) {
        city->buildQueue().cancel();
        return;
    }

    // Queue buildings at hovered tile
    if (!hoveredTile) {
        return;
    }
    int row = hoveredTile->row;
    int col = hoveredTile->col;

    if (IsKeyPressed(KEY_F)) {
        city->buildQueue().enqueue(game::makeFarm, row, col);
    } else if (IsKeyPressed(KEY_M)) {
        city->buildQueue().enqueue(game::makeMine, row, col);
    } else if (IsKeyPressed(KEY_K)) {
        city->buildQueue().enqueue(game::makeMarket, row, col);
    }
}

// ── Turn processing ──────────────────────────────────────────────────────────

static void processTurn(game::GameState &state) {
    state.nextTurn();

    // Reset unit movement and clear level-up indicators (skip dead units)
    for (auto &unit : state.units()) {
        if (unit->isAlive()) {
            unit->resetMovement();
            unit->clearLevelUpFlag();
        }
    }

    // Apply city production to build queues
    for (auto &city : state.cities()) {
        int prod = game::City::productionPerTurn();
        auto completed = city.buildQueue().applyProduction(prod);
        if (completed && std::holds_alternative<game::Building>(*completed)) {
            state.addBuilding(std::move(std::get<game::Building>(*completed)));
        }
    }

    // Collect building yields
    game::Resource totalYield;
    for (const auto &building : state.buildings()) {
        totalYield += building.yieldPerTurn();
    }
    state.factionResources() += totalYield;

    // Also add yields to the player faction's stockpile.
    auto *playerFaction = state.mutableFactionRegistry().findMutableFaction(PLAYER_FACTION_ID);
    if (playerFaction != nullptr) {
        playerFaction->addResources(totalYield);
    }

    // Recalculate fog of war at turn start (unit positions may have changed).
    state.recalculateFog(PLAYER_FACTION_ID);
}

// ── Save / Load ─────────────────────────────────────────────────────────

static void handleSaveLoad(game::GameState &state, int &selectedUnit) {
    if (IsKeyPressed(KEY_F5)) {
        auto path = game::generateSavePath();
        if (game::saveGame(state, path)) {
            TraceLog(LOG_INFO, "Game saved: %s", path.c_str());
        } else {
            TraceLog(LOG_ERROR, "Failed to save game");
        }
    }

    if (IsKeyPressed(KEY_F9)) {
        try {
            auto path = game::latestSavePath();
            state = game::loadGame(path);
            selectedUnit = NO_SELECTION;
            state.recalculateFog(PLAYER_FACTION_ID);
            TraceLog(LOG_INFO, "Game loaded: %s", path.c_str());
        } catch (const std::exception &e) {
            TraceLog(LOG_ERROR, "Failed to load game: %s", e.what());
        }
    }
}

// ── Unit click handling (select / attack) ────────────────────────────────────

/// Find the index of the first alive unit at the given tile.
static int findUnitAtTile(const game::GameState &state, int row, int col) {
    const auto &units = state.units();
    for (std::size_t i = 0; i < units.size(); ++i) {
        if (units[i]->isAlive() && units[i]->row() == row && units[i]->col() == col) {
            return static_cast<int>(i);
        }
    }
    return NO_SELECTION;
}

/// Try to execute an attack from selectedUnit to clickedUnit.
/// Spawns combat effects, logs the combat event, and triggers a camera cinematic.
/// Returns true if an attack was successfully executed.
static bool tryAttack(game::GameState &state, const engine::hex::TileCoord &tile, int &selectedUnit, int clickedUnit,
                      engine::CombatEffectManager &effects, game::CombatLog &combatLog,
                      engine::CameraCinematic &cinematic, const engine::Camera &camera,
                      engine::UnitAnimator &animator) {
    auto &units = state.units();
    auto &sel = units[selectedUnit];
    auto &target = units[clickedUnit];

    bool isEnemy =
        target->factionId() != sel->factionId() && state.diplomacy().areAtWar(sel->factionId(), target->factionId());
    if (!isEnemy) {
        return false;
    }

    // Capture unit info before combat (units may die during execute).
    std::string attackerName = sel->name();
    std::string defenderName = target->name();
    game::FactionId attackerFactionId = sel->factionId();
    game::FactionId defenderFactionId = target->factionId();
    int defenderRow = target->row();
    int defenderCol = target->col();
    int attackerRow = sel->row();
    int attackerCol = sel->col();

    // Look up faction names for the combat log.
    std::string attackerFactionName;
    std::string defenderFactionName;
    const auto *attackerFaction = state.factionRegistry().findFaction(attackerFactionId);
    if (attackerFaction != nullptr) {
        attackerFactionName = attackerFaction->name();
    }
    const auto *defenderFaction = state.factionRegistry().findFaction(defenderFactionId);
    if (defenderFaction != nullptr) {
        defenderFactionName = defenderFaction->name();
    }

    game::AttackAction action(static_cast<std::size_t>(selectedUnit), static_cast<std::size_t>(clickedUnit));
    auto result = action.execute(state);
    if (!result.executed) {
        return false;
    }

    // ── Spawn combat visual effects ──────────────────────────────────────

    // Hit flash on defender's tile.
    effects.spawnHitFlash(defenderRow, defenderCol);

    // Floating damage number on defender.
    Vector3 defCenter = engine::hex::tileCenter(defenderRow, defenderCol);
    effects.spawnDamageNumber(defCenter.x, DAMAGE_NUMBER_SPAWN_Y, defCenter.z, result.combat.damageToDefender,
                              DAMAGE_COLOR_NORMAL);

    // Counter-attack damage number on attacker (if any).
    if (result.combat.damageToAttacker > 0) {
        effects.spawnHitFlash(attackerRow, attackerCol);
        Vector3 atkCenter = engine::hex::tileCenter(attackerRow, attackerCol);
        effects.spawnDamageNumber(atkCenter.x + engine::COUNTER_DAMAGE_X_OFFSET, DAMAGE_NUMBER_SPAWN_Y, atkCenter.z,
                                  result.combat.damageToAttacker, DAMAGE_COLOR_COUNTER);
    }

    // Death effects.
    if (result.combat.defenderDied) {
        effects.spawnDeathEffect(defCenter.x, engine::DEATH_EFFECT_Y_OFFSET, defCenter.z, DAMAGE_COLOR_NORMAL);
    }
    if (result.combat.attackerDied) {
        Vector3 atkCenter = engine::hex::tileCenter(attackerRow, attackerCol);
        effects.spawnDeathEffect(atkCenter.x, engine::DEATH_EFFECT_Y_OFFSET, atkCenter.z, DAMAGE_COLOR_COUNTER);
    }

    // ── Trigger camera cinematic ────────────────────────────────────────

    cinematic.triggerCombat(defCenter, camera.snapshot());

    // ── Trigger unit animations ─────────────────────────────────────────

    // Attack lunge animation on the attacker.
    Vector3 atkAnimPos = engine::hex::tileCenter(attackerRow, attackerCol);
    Vector3 defAnimPos = engine::hex::tileCenter(defenderRow, defenderCol);
    animator.startAttack(static_cast<engine::UnitId>(selectedUnit),
                         {.x = atkAnimPos.x, .y = atkAnimPos.y, .z = atkAnimPos.z},
                         {.x = defAnimPos.x, .y = defAnimPos.y, .z = defAnimPos.z});

    // Death animations for killed units.
    if (result.combat.defenderDied) {
        animator.startDeath(static_cast<engine::UnitId>(clickedUnit),
                            {.x = defAnimPos.x, .y = defAnimPos.y, .z = defAnimPos.z});
    }
    if (result.combat.attackerDied) {
        animator.startDeath(static_cast<engine::UnitId>(selectedUnit),
                            {.x = atkAnimPos.x, .y = atkAnimPos.y, .z = atkAnimPos.z});
    }

    // ── Log combat event ─────────────────────────────────────────────────

    game::CombatEvent event;
    event.attackerUnitIndex = static_cast<std::size_t>(selectedUnit);
    event.defenderUnitIndex = static_cast<std::size_t>(clickedUnit);
    event.attackerFactionId = attackerFactionId;
    event.defenderFactionId = defenderFactionId;
    event.damageToDefender = result.combat.damageToDefender;
    event.damageToAttacker = result.combat.damageToAttacker;
    event.defenderDied = result.combat.defenderDied;
    event.attackerDied = result.combat.attackerDied;
    event.turn = state.getTurn();
    event.tileRow = defenderRow;
    event.tileCol = defenderCol;
    event.attackerName = attackerName;
    event.defenderName = defenderName;
    event.attackerFactionName = attackerFactionName;
    event.defenderFactionName = defenderFactionName;
    combatLog.append(event);

    // Recalculate fog of war after combat (units may have died).
    state.recalculateFog(PLAYER_FACTION_ID);

    // ── Update selection ─────────────────────────────────────────────────

    // If attacker died, deselect.
    if (result.combat.attackerDied) {
        selectedUnit = NO_SELECTION;
    } else if (result.combat.defenderDied &&
               static_cast<std::size_t>(clickedUnit) < static_cast<std::size_t>(selectedUnit)) {
        // Defender had a lower index and was removed — our index shifted down.
        --selectedUnit;
    }
    return true;
}

/// Try to move the selected unit to the clicked tile (adjacent, empty).
/// Returns true if movement occurred.
static bool tryMoveUnit(game::GameState &state, const engine::hex::TileCoord &tile, int selectedUnit,
                        game::CombatLog &combatLog, engine::UnitAnimator &animator) {
    // Capture the unit's current tile position *before* the move executes.
    const auto &unit = state.units().at(selectedUnit);
    int fromRow = unit->row();
    int fromCol = unit->col();

    game::MoveAction action(static_cast<std::size_t>(selectedUnit), tile.row, tile.col);
    auto result = action.execute(state);
    if (!result.executed) {
        return false;
    }

    // Start the movement animation from the old tile to the new tile.
    Vector3 fromCenter = engine::hex::tileCenter(fromRow, fromCol);
    Vector3 toCenter = engine::hex::tileCenter(tile.row, tile.col);
    animator.startMove(static_cast<engine::UnitId>(selectedUnit),
                       {.x = fromCenter.x, .y = fromCenter.y, .z = fromCenter.z},
                       {.x = toCenter.x, .y = toCenter.y, .z = toCenter.z});

    // Recalculate fog of war after unit movement.
    state.recalculateFog(PLAYER_FACTION_ID);

    // After movement, attempt city capture.
    game::CaptureAction captureAction(static_cast<std::size_t>(selectedUnit));
    auto captureResult = captureAction.execute(state);
    if (captureResult.executed) {
        // Log the capture event.
        game::CombatEvent captureEvent;
        captureEvent.isCaptureEvent = true;
        captureEvent.capturedCityName = captureResult.capturedCityName;
        captureEvent.attackerFactionId = captureResult.newOwner;
        captureEvent.defenderFactionId = captureResult.previousOwner;
        captureEvent.turn = state.getTurn();
        captureEvent.tileRow = tile.row;
        captureEvent.tileCol = tile.col;

        // Look up faction names for the log.
        const auto *capturerFaction = state.factionRegistry().findFaction(captureResult.newOwner);
        if (capturerFaction != nullptr) {
            captureEvent.attackerFactionName = capturerFaction->name();
        }
        const auto *previousFaction = state.factionRegistry().findFaction(captureResult.previousOwner);
        if (previousFaction != nullptr) {
            captureEvent.defenderFactionName = previousFaction->name();
        }

        combatLog.append(captureEvent);
    }

    return true;
}

/// Handle click when a unit is already selected.
/// Returns true if the click was consumed.
static bool handleSelectedUnitClick(game::GameState &state, const engine::hex::TileCoord &tile, int &selectedUnit,
                                    int clickedUnit, engine::CombatEffectManager &effects, game::CombatLog &combatLog,
                                    engine::CameraCinematic &cinematic, const engine::Camera &camera,
                                    engine::UnitAnimator &animator) {
    auto &units = state.units();
    auto &sel = units[selectedUnit];

    // Only handle actions for player-owned units.
    if (sel->factionId() != PLAYER_FACTION_ID) {
        if (clickedUnit != NO_SELECTION) {
            selectedUnit = clickedUnit;
            return true;
        }
        selectedUnit = NO_SELECTION;
        return false;
    }

    // Click on another unit — try attack or switch selection.
    if (clickedUnit != NO_SELECTION && clickedUnit != selectedUnit) {
        if (tryAttack(state, tile, selectedUnit, clickedUnit, effects, combatLog, cinematic, camera, animator)) {
            return true;
        }
        // Click on a friendly unit — select it instead.
        auto &target = units[clickedUnit];
        if (target->factionId() == sel->factionId()) {
            selectedUnit = clickedUnit;
            return true;
        }
    }

    // Click on empty tile — try to move.
    if (clickedUnit == NO_SELECTION && tryMoveUnit(state, tile, selectedUnit, combatLog, animator)) {
        return true;
    }

    // Click on same tile — deselect.
    if (clickedUnit == selectedUnit) {
        selectedUnit = NO_SELECTION;
        return true;
    }

    return false;
}

/// Handle a left-click on a tile for unit selection, movement, or attack.
/// Returns true if the click was consumed by unit logic.
static bool handleUnitClick(game::GameState &state, const engine::hex::TileCoord &tile, int &selectedUnit,
                            engine::CombatEffectManager &effects, game::CombatLog &combatLog,
                            engine::CameraCinematic &cinematic, const engine::Camera &camera,
                            engine::UnitAnimator &animator) {
    int clickedUnit = findUnitAtTile(state, tile.row, tile.col);

    if (selectedUnit != NO_SELECTION) {
        auto &units = state.units();
        if (static_cast<std::size_t>(selectedUnit) >= units.size() || !units[selectedUnit]->isAlive()) {
            selectedUnit = NO_SELECTION;
            return false;
        }
        return handleSelectedUnitClick(state, tile, selectedUnit, clickedUnit, effects, combatLog, cinematic, camera,
                                       animator);
    }

    // No unit selected — try to select one.
    if (clickedUnit != NO_SELECTION) {
        selectedUnit = clickedUnit;
        return true;
    }

    return false;
}

// ── Input handling ────────────────────────────────────────────────────────────

static void handleInput(game::GameState &state, const std::optional<engine::hex::TileCoord> &hoveredTile,
                        int &selectedUnit, std::optional<game::CityId> &selectedCity,
                        engine::CombatEffectManager &effects, game::CombatLog &combatLog, int &combatLogScroll,
                        engine::CameraCinematic &cinematic, const engine::Camera &camera,
                        engine::UnitAnimator &animator) {
    // Clean up any dead units and adjust selected unit index accordingly.
    state.removeDeadUnits(&selectedUnit);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoveredTile) {
        bool unitHandled =
            handleUnitClick(state, *hoveredTile, selectedUnit, effects, combatLog, cinematic, camera, animator);
        if (!unitHandled) {
            handleCityClick(state.cities(), *hoveredTile, selectedCity);
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE)) {
        selectedUnit = NO_SELECTION;
        selectedCity = std::nullopt;
    }

    if (selectedCity) {
        handleBuildActions(state, *selectedCity, hoveredTile);
    }

    if (IsKeyPressed(KEY_SPACE)) {
        processTurn(state);
    }

    // F1 toggles debug mode (disables fog of war).
    if (IsKeyPressed(KEY_F1)) {
        DEBUG_NO_FOG = !DEBUG_NO_FOG;
    }

    // Combat log scrolling with Page Up / Page Down.
    if (IsKeyPressed(KEY_PAGE_UP)) {
        ++combatLogScroll;
    }
    if (IsKeyPressed(KEY_PAGE_DOWN) && combatLogScroll > 0) {
        --combatLogScroll;
    }
}

// ── Rendering ────────────────────────────────────────────────────────────────

static void renderGameFrame(const game::GameState &state, Camera3D cam,
                            const std::optional<engine::hex::TileCoord> &hoveredTile, int selectedUnit,
                            std::optional<game::CityId> selectedCity, const engine::ModelManager &models,
                            engine::CombatEffectManager &effects, const game::CombatLog &combatLog, int combatLogScroll,
                            const std::vector<game::ReachableTile> &movementRange, const engine::Tooltip &tooltip,
                            const engine::EndTurnButton &endTurnBtn, const engine::UnitAnimator &animator) {
    BeginMode3D(cam);
    const auto *fog = DEBUG_NO_FOG ? nullptr : &state.fogOfWar();
    engine::drawMap(state.map(), hoveredTile, fog, PLAYER_FACTION_ID);

    // Draw movement range overlay for selected unit.
    if (selectedUnit != NO_SELECTION) {
        engine::drawMovementRangeOverlay(movementRange);
    }

    // Draw attack range overlay for selected ranged unit.
    if (selectedUnit != NO_SELECTION) {
        const auto &selUnit = state.units().at(selectedUnit);
        if (selUnit->isAlive()) {
            engine::drawAttackRangeOverlay(*selUnit, state.map());
        }
    }

    engine::drawUnits(state.units(), state.factionRegistry(), models, selectedUnit, PLAYER_FACTION_ID,
                      &state.diplomacy(), fog, &animator);
    engine::drawCities(state.cities(), state.factionRegistry(), selectedCity, PLAYER_FACTION_ID,
                       &state.diplomacy(), fog);
    engine::drawBuildings(state.buildings(), models, state.factionRegistry(), fog, PLAYER_FACTION_ID);

    // Draw 3D combat effects (hit flashes and death effects).
    effects.drawHitFlashes();
    effects.drawDeathEffects();

    EndMode3D();

    // Draw 2D overlay effects (floating damage numbers, projected from 3D).
    effects.drawDamageNumbers(cam);

    // Draw HUD panels: unit info panel (replaces old top HUD when unit selected)
    // or compact resource HUD when no unit is selected.
    if (selectedUnit != NO_SELECTION) {
        drawUnitInfoPanel(state, selectedUnit);
    } else {
        drawTopHud(state);
    }

    if (selectedCity) {
        for (const auto &city : state.cities()) {
            if (city.id() == *selectedCity) {
                drawCityPanel(city, state, hoveredTile);
                break;
            }
        }
    }

    drawFactionListPanel(state);

    // Draw combat log panel.
    engine::drawCombatLogPanel(combatLog, SCREEN_WIDTH, SCREEN_HEIGHT, combatLogScroll);

    // Draw minimap overlay.
    engine::minimap::draw(state.map(), fog, PLAYER_FACTION_ID, state.units(), state.cities(),
                          state.factionRegistry(), cam, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Draw end-turn button.
    endTurnBtn.draw(SCREEN_WIDTH, SCREEN_HEIGHT, state.getTurn());

    // Draw tooltip (on top of everything).
    tooltip.draw(state, PLAYER_FACTION_ID);

    // Debug mode indicator.
    if (DEBUG_NO_FOG) {
        DrawText("[DEBUG] Fog of War OFF (F1)", 14, SCREEN_HEIGHT - 30, 16, RED);
    }
}

// ── All mutable game-loop state, bundled to keep main() simple ───────────────

struct AppState {
    engine::Screen currentScreen = engine::Screen::MainMenu;
    engine::Screen pendingScreen = engine::Screen::MainMenu;
    engine::MainMenu mainMenu;
    engine::SettingsScreen settingsScreen;
    engine::Transition transition;
    bool gameInitialized = false;
    bool quitRequested = false;

    game::GameState state{MAP_ROWS, MAP_COLS};
    engine::Camera camera;
    int selectedUnit = NO_SELECTION;
    std::optional<game::CityId> selectedCity;

    engine::ModelManager modelManager;
    engine::CombatEffectManager combatEffects;
    engine::CameraCinematic cinematic;
    engine::UnitAnimator unitAnimator;
    game::CombatLog combatLog;
    int combatLogScroll = 0;

    std::vector<game::ReachableTile> movementRange;

    engine::Tooltip tooltip;
    engine::EndTurnButton endTurnBtn;
};

// ── Per-screen update + draw helpers ─────────────────────────────────────────

static void updateMainMenu(AppState &app, int screenW, int screenH) {
    app.mainMenu.draw(screenW, screenH);
    if (app.transition.isActive()) {
        return;
    }
    auto action = app.mainMenu.update(screenW, screenH);
    if (action == engine::MenuAction::NewGame) {
        app.pendingScreen = engine::Screen::InGame;
        app.transition.start(engine::TransitionDir::FadeOut);
    } else if (action == engine::MenuAction::Settings) {
        app.pendingScreen = engine::Screen::Settings;
        app.transition.start(engine::TransitionDir::FadeOut);
    } else if (action == engine::MenuAction::Quit) {
        app.quitRequested = true;
    }
}

static void updateSettings(AppState &app, int screenW, int screenH) {
    app.settingsScreen.draw(screenW, screenH);
    if (app.transition.isActive()) {
        return;
    }
    auto action = app.settingsScreen.update(screenW, screenH);
    if (action == engine::MenuAction::BackToMenu) {
        app.pendingScreen = engine::Screen::MainMenu;
        app.transition.start(engine::TransitionDir::FadeOut);
    }
}

static void updateInGame(AppState &app, float dt) {
    // Update cinematic if active; otherwise allow normal camera input.
    if (app.cinematic.isActive()) {
        // Allow player to skip the cinematic with Escape or Space.
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
            app.cinematic.skip();
            app.camera.restoreFromSnapshot(app.cinematic.currentSnapshot());
        } else {
            app.cinematic.update(dt);
            // When cinematic finishes naturally, restore camera state.
            if (!app.cinematic.isActive()) {
                app.camera.restoreFromSnapshot(app.cinematic.currentSnapshot());
            }
        }
    } else {
        app.camera.update();
    }

    // Use cinematic camera during playback, normal camera otherwise.
    Camera3D cam = app.cinematic.isActive() ? app.cinematic.getCamera() : app.camera.raw();
    auto hoveredTile = engine::input::mouseToTile(cam, MAP_ROWS, MAP_COLS);

    app.combatEffects.update(dt);
    app.unitAnimator.update(dt);
    app.unitAnimator.removeFinished();

    if (hoveredTile) {
        app.tooltip.update(hoveredTile->row, hoveredTile->col, true, dt);
    } else {
        app.tooltip.update(0, 0, false, dt);
    }

    // Disable player input during cinematic.
    if (!app.transition.isActive() && !app.cinematic.isActive()) {
        handleInput(app.state, hoveredTile, app.selectedUnit, app.selectedCity, app.combatEffects, app.combatLog,
                    app.combatLogScroll, app.cinematic, app.camera, app.unitAnimator);
        handleSaveLoad(app.state, app.selectedUnit);
        if (app.endTurnBtn.update(SCREEN_WIDTH, SCREEN_HEIGHT)) {
            processTurn(app.state);
        }
    }

    app.movementRange.clear();
    if (app.selectedUnit != NO_SELECTION) {
        const auto &unit = app.state.units().at(app.selectedUnit);
        if (unit->isAlive() && unit->factionId() == PLAYER_FACTION_ID) {
            app.movementRange = game::computeMovementRange(unit->row(), unit->col(), unit->movementRemaining(),
                                                           app.state.map(), app.state.registry(), unit->factionId());
        }
    }

    renderGameFrame(app.state, cam, hoveredTile, app.selectedUnit, app.selectedCity, app.modelManager,
                    app.combatEffects, app.combatLog, app.combatLogScroll, app.movementRange, app.tooltip,
                    app.endTurnBtn, app.unitAnimator);
}

static void handleTransitionComplete(AppState &app) {
    app.currentScreen = app.pendingScreen;
    app.transition.start(engine::TransitionDir::FadeIn);
    if (app.currentScreen == engine::Screen::InGame && !app.gameInitialized) {
        setupDemoState(app.state);

        // Register procedural fallback models for each unit type.
        app.modelManager.generateFallback("unit_warrior", engine::FallbackShape::Cube);
        app.modelManager.generateFallback("unit_archer", engine::FallbackShape::Cone);
        app.modelManager.generateFallback("unit_settler", engine::FallbackShape::Sphere);

        // Register procedural fallback models for each building type.
        engine::registerBuildingModels(app.modelManager);

        app.gameInitialized = true;
    }
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main() {
    engine::window::init(SCREEN_WIDTH, SCREEN_HEIGHT, "My4X");

    AppState app;
    app.transition.start(engine::TransitionDir::FadeIn);

    while (engine::window::isRunning()) {
        float dt = GetFrameTime();
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        app.transition.update(dt);

        if (app.transition.isComplete() && app.transition.alpha() >= 1.0F) {
            handleTransitionComplete(app);
        }

        engine::window::beginFrame();

        if (app.currentScreen == engine::Screen::MainMenu) {
            updateMainMenu(app, screenW, screenH);
        } else if (app.currentScreen == engine::Screen::Settings) {
            updateSettings(app, screenW, screenH);
        } else {
            updateInGame(app, dt);
        }

        app.transition.draw(screenW, screenH);
        engine::window::endFrame();

        if (app.quitRequested) {
            break;
        }
    }

    engine::window::shutdown();
    return 0;
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-pro-type-vararg)
