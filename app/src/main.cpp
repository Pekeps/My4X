// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-pro-type-vararg)

#include "engine/BuildingRenderer.h"
#include "engine/Camera.h"
#include "engine/CityRenderer.h"
#include "engine/Input.h"
#include "engine/MapRenderer.h"
#include "engine/UnitRenderer.h"
#include "engine/Window.h"
#include "game/BuildQueue.h"
#include "game/Building.h"
#include "game/City.h"
#include "game/GameState.h"
#include "game/SaveLoad.h"
#include "game/TerrainType.h"
#include "game/UnitTypeRegistry.h"
#include "game/Warrior.h"

#include "raylib.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

// ── Screen & map ─────────────────────────────────────────────────────────────

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int MAP_ROWS = 30;
const int MAP_COLS = 12;

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

// ── Demo setup ───────────────────────────────────────────────────────────────

static void setupDemoState(game::GameState &state) {
    game::City capital("Ironhold", 5, 3);
    capital.addTile(5, 4);
    capital.addTile(4, 3);
    state.addCity(std::move(capital));
    state.addBuilding(game::makeFarm(5, 4));
    state.factionResources() += game::Resource{.gold = 50, .production = 20, .food = 30};
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

// ── Top-left HUD ─────────────────────────────────────────────────────────────

static void drawTopHud(const game::GameState &state, int selectedUnit) {
    // Background
    int hudH = 78;
    if (selectedUnit != NO_SELECTION) {
        hudH += HUD_LINE_H;
    }
    DrawRectangleRounded(
        {static_cast<float>(HUD_X - 6), static_cast<float>(HUD_Y - 4), 420.0F, static_cast<float>(hudH)}, 0.08F, 4,
        HUD_BG);

    int y = HUD_Y;

    // Turn
    std::string turnText = "My4X  --  Turn " + std::to_string(state.getTurn());
    DrawText(turnText.c_str(), HUD_X, y, HUD_TITLE_SIZE, HUD_TITLE_COL);
    y += HUD_TITLE_SIZE + 6;

    // Resources
    const auto &res = state.factionResources();
    std::string goldStr = "Gold: " + std::to_string(res.gold);
    std::string prodStr = "Prod: " + std::to_string(res.production);
    std::string foodStr = "Food: " + std::to_string(res.food);
    DrawText(goldStr.c_str(), HUD_X, y, HUD_TEXT_SIZE, HUD_ACCENT_COL);
    DrawText(prodStr.c_str(), HUD_X + 110, y, HUD_TEXT_SIZE, HUD_TEXT_COL);
    DrawText(foodStr.c_str(), HUD_X + 220, y, HUD_TEXT_SIZE, Color{120, 200, 100, 255});
    y += HUD_LINE_H;

    // Controls hint
    DrawText("LMB select  |  RMB deselect  |  SPACE next turn", HUD_X, y, PANEL_KEY_SIZE, HUD_DIM_COL);
    y += HUD_LINE_H;

    // Selected unit
    if (selectedUnit != NO_SELECTION) {
        const auto &unit = state.units().at(selectedUnit);
        std::string info = unit->name() + "  HP " + std::to_string(unit->health()) + "/" +
                           std::to_string(unit->maxHealth()) + "  Moves " + std::to_string(unit->movementRemaining()) +
                           "/" + std::to_string(unit->movement());
        DrawText(info.c_str(), HUD_X, y, HUD_TEXT_SIZE, YELLOW);
    }
}

// ── City info panel ──────────────────────────────────────────────────────────

static void drawCityPanel(const game::City &city, std::optional<engine::hex::TileCoord> hoveredTile) {
    int contentW = PANEL_W - (PANEL_PAD * 2);
    int x = PANEL_X + PANEL_PAD;

    // Measure panel height dynamically
    int panelH = PANEL_PAD;                              // top padding
    panelH += PANEL_TITLE_SIZE + 8;                      // city name + gap
    panelH += 1 + PANEL_SECTION_GAP;                     // separator
    panelH += PANEL_LINE_H * 3;                          // stats: population, territory, production
    panelH += PANEL_SECTION_GAP + 1 + PANEL_SECTION_GAP; // separator
    panelH += PANEL_HEADING_SIZE + 6;                    // "BUILD QUEUE" heading

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
    std::string factionStr = "Faction " + std::to_string(city.factionId());
    int factionW = MeasureText(factionStr.c_str(), PANEL_KEY_SIZE);
    DrawText(factionStr.c_str(), PANEL_X + PANEL_W - PANEL_PAD - factionW, y + 4, PANEL_KEY_SIZE, PANEL_DIM_COL);
    y += PANEL_TITLE_SIZE + 8;

    drawSeparator(x, y, contentW);
    y += 1 + PANEL_SECTION_GAP;

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

    // Reset unit movement
    for (auto &unit : state.units()) {
        unit->resetMovement();
    }

    // Apply city production to build queues
    for (auto &city : state.cities()) {
        int prod = game::City::productionPerTurn();
        auto completed = city.buildQueue().applyProduction(prod);
        if (completed) {
            state.addBuilding(std::move(*completed));
        }
    }

    // Collect building yields
    game::Resource totalYield;
    for (const auto &building : state.buildings()) {
        totalYield += building.yieldPerTurn();
    }
    state.factionResources() += totalYield;
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
            TraceLog(LOG_INFO, "Game loaded: %s", path.c_str());
        } catch (const std::exception &e) {
            TraceLog(LOG_ERROR, "Failed to load game: %s", e.what());
        }
    }
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main() {
    engine::window::init(SCREEN_WIDTH, SCREEN_HEIGHT, "My4X");

    game::GameState state(MAP_ROWS, MAP_COLS);
    engine::Camera camera;

    int selectedUnit = NO_SELECTION;
    std::optional<game::CityId> selectedCity;

    setupDemoState(state);

    while (engine::window::isRunning()) {
        camera.update();
        Camera3D cam = camera.raw();
        auto hoveredTile = engine::input::mouseToTile(cam, MAP_ROWS, MAP_COLS);

        // ── Input ────────────────────────────────────────────────────────
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoveredTile) {
            handleCityClick(state.cities(), *hoveredTile, selectedCity);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE)) {
            selectedCity = std::nullopt;
        }

        if (selectedCity) {
            handleBuildActions(state, *selectedCity, hoveredTile);
        }

        if (IsKeyPressed(KEY_SPACE)) {
            processTurn(state);
        }

        // ── Render ───────────────────────────────────────────────────────
        engine::window::beginFrame();

        BeginMode3D(cam);
        engine::drawMap(state.map(), hoveredTile);
        engine::drawUnits(state.units(), state.factionRegistry(), selectedUnit);
        engine::drawCities(state.cities(), state.factionRegistry(), selectedCity);
        engine::drawBuildings(state.buildings());
        EndMode3D();

        drawTopHud(state, selectedUnit);

        if (selectedCity) {
            for (const auto &city : state.cities()) {
                if (city.id() == *selectedCity) {
                    drawCityPanel(city, hoveredTile);
                    break;
                }
            }
        }

        handleSaveLoad(state, selectedUnit);

        engine::window::endFrame();
    }

    engine::window::shutdown();
    return 0;
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-pro-type-vararg)