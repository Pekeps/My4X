# Phase 2: Factions & Units — Execution Plan

## Overview

Faction system with player and neutral factions, data-driven unit types (Settler, Scout, Warrior), faction ownership of entities, diplomacy, and faction-aware rendering/UI. Depends on Phase 1 (complete).

## Task Dependency Graph

```
Layer 1 (parallel — no dependencies)
├── TON-25: Faction data model
└── TON-31: UnitTemplate registry
         │
Layer 2   │
├── TON-26: FactionRegistry ──────────── depends on: TON-25
│          │
Layer 3    │  (parallel — all depend on TON-26)
├── TON-27: Units + factions ─────────── depends on: TON-26
├── TON-28: Cities + factions ────────── depends on: TON-26
└── TON-30: Diplomacy state machine ──── depends on: TON-26
         │
Layer 4   │  (parallel — mixed deps from Layer 3 + Layer 1)
├── TON-29: Faction colors ──────────── depends on: TON-27, TON-28
├── TON-32: Unit production from cities ─ depends on: TON-27, TON-28, TON-31
└── TON-33: Neutral spawning & basic AI ─ depends on: TON-27, TON-30
         │
Layer 5   │
└── TON-34: Faction-aware UI ────────── depends on: TON-29, TON-30
```

## Dependency Matrix

| Task   | Blocked by                | Blocks              |
|--------|--------------------------|----------------------|
| TON-25 | —                        | TON-26               |
| TON-31 | —                        | TON-32               |
| TON-26 | TON-25                   | TON-27, TON-28, TON-30 |
| TON-27 | TON-26                   | TON-29, TON-32, TON-33 |
| TON-28 | TON-26                   | TON-29, TON-32       |
| TON-30 | TON-26                   | TON-33, TON-34       |
| TON-29 | TON-27, TON-28           | TON-34               |
| TON-32 | TON-27, TON-28, TON-31   | —                    |
| TON-33 | TON-27, TON-30           | —                    |
| TON-34 | TON-29, TON-30           | —                    |

## Execution Order

### Layer 1 — Foundation (parallel)

**TON-25: Design and implement Faction data model**
- `Faction` class in `game/`: ID, name, `FactionType` enum (Player/NeutralHostile/NeutralPassive), color index, resource stockpile
- No engine/raylib dependencies

**TON-31: Design unit type definition system (UnitTemplate registry)**
- `UnitTemplate` struct: name, maxHealth, attack, defense, movementRange, productionCost, attackRange
- `UnitTypeRegistry` with registration and lookup
- Refactor `Warrior` to use template; define 3 unit types (Warrior, Archer, Settler)

### Layer 2 — Registry

**TON-26: Implement FactionRegistry to manage all factions**
- `FactionRegistry`: owns all factions, add/get/iterate/filter
- Integrated into `GameState`
- *Blocked by: TON-25*

### Layer 3 — Ownership & Diplomacy (parallel)

**TON-27: Associate units with factions (ownership)**
- `Unit::factionId()` accessor, immutable at construction
- Units queryable by faction in GameState
- *Blocked by: TON-26*

**TON-28: Associate cities with factions (ownership)**
- `City::factionId()` + `setFactionId()` (for future capture)
- `TurnResolver` routes city production to owning faction's stockpile
- *Blocked by: TON-26*

**TON-30: Implement diplomacy state machine**
- `DiplomacyState` enum: War, Peace, Alliance
- `DiplomacyManager`: pairwise symmetric relations, default rules by faction type
- *Blocked by: TON-26*

### Layer 4 — Integration (parallel)

**TON-29: Implement faction color system and visual identity**
- `FactionColors` utility in `engine/` mapping color index to raylib Color
- Update UnitRenderer and CityRenderer to use faction colors
- *Blocked by: TON-27, TON-28*

**TON-32: Implement unit production from cities**
- `ProduceUnitOrder` in city build queue
- Resource cost deduction, unit spawning near city on completion
- *Blocked by: TON-27, TON-28, TON-31*

**TON-33: Implement neutral faction spawning and basic AI**
- Spawn neutral factions with units during game init
- `NeutralAI` class: hostile guard behavior, passive peace-until-attacked
- *Blocked by: TON-27, TON-30*
- *Note: Full hostile combat behavior requires Phase 3; implement guard/decision logic now, combat resolution later*

### Layer 5 — Polish

**TON-34: Add faction-aware UI elements**
- Unit selection panel with faction name/color, city panel with faction info
- Diplomacy indicators on foreign units/cities, player resource stockpile in HUD
- *Blocked by: TON-29, TON-30*

## Maximum Parallelism Strategy

If dispatching agents in parallel, the optimal schedule is:

| Step | Tasks (parallel)        | Wait for              |
|------|-------------------------|-----------------------|
| 1    | TON-25, TON-31          | —                     |
| 2    | TON-26                  | TON-25                |
| 3    | TON-27, TON-28, TON-30  | TON-26                |
| 4    | TON-29, TON-32, TON-33  | TON-27+28, TON-27+30, TON-31 |
| 5    | TON-34                  | TON-29, TON-30        |

**Critical path:** TON-25 → TON-26 → TON-27 → TON-29 → TON-34 (5 sequential steps)
