// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "game/Combat.h"
#include "game/Unit.h"
#include "game/UnitTypeRegistry.h"

#include <gtest/gtest.h>

namespace {

constexpr game::FactionId FACTION_A = 1;
constexpr game::FactionId FACTION_B = 2;

/// Build a registry with the default unit types.
game::UnitTypeRegistry makeRegistry() {
    game::UnitTypeRegistry reg;
    reg.registerDefaults();
    return reg;
}

/// Register a custom unit template for testing specific stat combinations.
void registerCustom(game::UnitTypeRegistry &reg, const std::string &name, int maxHealth, int attack, int defense,
                    int attackRange) {
    game::UnitTemplate tmpl;
    tmpl.name = name;
    tmpl.maxHealth = maxHealth;
    tmpl.attack = attack;
    tmpl.defense = defense;
    tmpl.movementRange = 2;
    tmpl.attackRange = attackRange;
    reg.registerTemplate(name, tmpl);
}

} // namespace

// ── Normal combat between two warriors ──────────────────────────────────────

TEST(CombatTest, WarriorVsWarrior_NormalDamage) {
    auto reg = makeRegistry();
    // Warrior: attack=15, defense=10, hp=100, melee
    game::Unit attacker(0, 0, reg.getTemplate("Warrior"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("Warrior"), FACTION_B);

    auto result = game::resolveCombat(attacker, defender);

    // damage = max(1, 15 - 10/2) = max(1, 10) = 10
    EXPECT_EQ(result.damageToDefender, 10);
    EXPECT_FALSE(result.defenderDied);

    // Counter-attack: defender is melee and survived, so it strikes back.
    // counter damage = max(1, 15 - 10/2) = 10
    EXPECT_EQ(result.damageToAttacker, 10);
    EXPECT_FALSE(result.attackerDied);
}

// ── One-shot kill (defender dies in a single strike) ────────────────────────

TEST(CombatTest, OneShotKill_DefenderDies) {
    auto reg = makeRegistry();
    registerCustom(reg, "Tank", 200, 60, 5, 1);
    registerCustom(reg, "Scout", 10, 3, 2, 1);

    game::Unit attacker(0, 0, reg.getTemplate("Tank"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("Scout"), FACTION_B);

    auto result = game::resolveCombat(attacker, defender);

    // damage = max(1, 60 - 2/2) = 59. Scout has 10 HP, so clamped to 10.
    EXPECT_EQ(result.damageToDefender, 10);
    EXPECT_TRUE(result.defenderDied);

    // Defender died, so no counter-attack.
    EXPECT_EQ(result.damageToAttacker, 0);
    EXPECT_FALSE(result.attackerDied);
}

// ── Defender counter-kills attacker ─────────────────────────────────────────

TEST(CombatTest, CounterKill_AttackerDies) {
    auto reg = makeRegistry();
    // Weak attacker with low HP attacks a strong defender.
    registerCustom(reg, "WeakAttacker", 5, 10, 2, 1);
    registerCustom(reg, "StrongDefender", 200, 50, 8, 1);

    game::Unit attacker(0, 0, reg.getTemplate("WeakAttacker"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("StrongDefender"), FACTION_B);

    auto result = game::resolveCombat(attacker, defender);

    // damage to defender = max(1, 10 - 8/2) = max(1, 6) = 6
    EXPECT_EQ(result.damageToDefender, 6);
    EXPECT_FALSE(result.defenderDied);

    // counter = max(1, 50 - 2/2) = max(1, 49) = 49, clamped to attacker HP=5
    EXPECT_EQ(result.damageToAttacker, 5);
    EXPECT_TRUE(result.attackerDied);
}

// ── Minimum damage floor ────────────────────────────────────────────────────

TEST(CombatTest, MinimumDamageFloor) {
    auto reg = makeRegistry();
    // Attacker has 1 attack, defender has massive defense.
    registerCustom(reg, "Peashooter", 100, 1, 0, 1);
    registerCustom(reg, "Fortress", 100, 1, 200, 1);

    game::Unit attacker(0, 0, reg.getTemplate("Peashooter"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("Fortress"), FACTION_B);

    auto result = game::resolveCombat(attacker, defender);

    // damage = max(1, 1 - 200/2) = max(1, -99) = 1
    EXPECT_EQ(result.damageToDefender, 1);
    EXPECT_FALSE(result.defenderDied);
}

// ── Ranged defender does NOT counter-attack ─────────────────────────────────

TEST(CombatTest, RangedDefenderNoCounterAttack) {
    auto reg = makeRegistry();
    // Archer: attack=12, defense=5, hp=70, range=3
    game::Unit attacker(0, 0, reg.getTemplate("Warrior"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("Archer"), FACTION_B);

    auto result = game::resolveCombat(attacker, defender);

    // damage = max(1, 15 - 5/2) = max(1, 13) = 13
    EXPECT_EQ(result.damageToDefender, 13);
    EXPECT_FALSE(result.defenderDied);

    // Archer is ranged (attackRange=3), so no counter-attack.
    EXPECT_EQ(result.damageToAttacker, 0);
    EXPECT_FALSE(result.attackerDied);
}

// ── Terrain defense bonus ───────────────────────────────────────────────────

TEST(CombatTest, TerrainDefenseBonus_ReducesDamage) {
    auto reg = makeRegistry();
    game::Unit attacker(0, 0, reg.getTemplate("Warrior"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("Warrior"), FACTION_B);

    game::CombatContext context;
    context.terrainDefenseBonus = 6; // e.g. hills

    auto result = game::resolveCombat(attacker, defender, context);

    // effectiveDefense = 10 + 6 = 16
    // damage = max(1, 15 - 16/2) = max(1, 7) = 7
    EXPECT_EQ(result.damageToDefender, 7);
    EXPECT_FALSE(result.defenderDied);
}

// ── Attacker terrain defense bonus reduces counter-attack damage ────────────

TEST(CombatTest, AttackerTerrainDefenseBonus_ReducesCounterDamage) {
    auto reg = makeRegistry();
    game::Unit attacker(0, 0, reg.getTemplate("Warrior"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("Warrior"), FACTION_B);

    game::CombatContext context;
    context.attackerTerrainDefenseBonus = 4;

    auto result = game::resolveCombat(attacker, defender, context);

    // Primary attack is unmodified: damage = max(1, 15 - 10/2) = 10
    EXPECT_EQ(result.damageToDefender, 10);

    // Counter-attack: attacker effective defense = 10 + 4 = 14
    // counter = max(1, 15 - 14/2) = max(1, 8) = 8
    EXPECT_EQ(result.damageToAttacker, 8);
}

// ── Pure function: units are not mutated ────────────────────────────────────

TEST(CombatTest, PureFunction_UnitsUnchanged) {
    auto reg = makeRegistry();
    game::Unit attacker(0, 0, reg.getTemplate("Warrior"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("Warrior"), FACTION_B);

    const int attackerHpBefore = attacker.health();
    const int defenderHpBefore = defender.health();

    game::resolveCombat(attacker, defender);

    EXPECT_EQ(attacker.health(), attackerHpBefore);
    EXPECT_EQ(defender.health(), defenderHpBefore);
}

// ── Default context has no modifiers ────────────────────────────────────────

TEST(CombatTest, DefaultContext_NoModifiers) {
    game::CombatContext ctx;
    EXPECT_EQ(ctx.terrainDefenseBonus, 0);
    EXPECT_EQ(ctx.attackerTerrainDefenseBonus, 0);
    EXPECT_FALSE(ctx.rangedAttack);
}

// ── Zero attack still deals minimum damage ──────────────────────────────────

TEST(CombatTest, ZeroAttack_StillDealsMinimumDamage) {
    auto reg = makeRegistry();
    // Settler: attack=0, defense=3, hp=50
    game::Unit attacker(0, 0, reg.getTemplate("Settler"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("Warrior"), FACTION_B);

    auto result = game::resolveCombat(attacker, defender);

    // damage = max(1, 0 - 10/2) = max(1, -5) = 1
    EXPECT_EQ(result.damageToDefender, 1);
}

// ── Ranged attack suppresses melee counter-attack ───────────────────────────

TEST(CombatTest, RangedAttackFlag_SuppressesCounterAttack) {
    auto reg = makeRegistry();
    // Warrior (melee) attacking from range (via context flag).
    game::Unit attacker(0, 0, reg.getTemplate("Warrior"), FACTION_A);
    game::Unit defender(3, 0, reg.getTemplate("Warrior"), FACTION_B);

    game::CombatContext context;
    context.rangedAttack = true;

    auto result = game::resolveCombat(attacker, defender, context);

    // Damage to defender should still be dealt.
    EXPECT_GT(result.damageToDefender, 0);

    // Counter-attack suppressed because rangedAttack = true.
    EXPECT_EQ(result.damageToAttacker, 0);
    EXPECT_FALSE(result.attackerDied);
}

// ── Damage clamped to defender's remaining health ───────────────────────────

TEST(CombatTest, DamageClampedToRemainingHealth) {
    auto reg = makeRegistry();
    registerCustom(reg, "Glass", 3, 5, 0, 1);
    registerCustom(reg, "Brute", 200, 100, 0, 1);

    game::Unit attacker(0, 0, reg.getTemplate("Brute"), FACTION_A);
    game::Unit defender(1, 0, reg.getTemplate("Glass"), FACTION_B);

    auto result = game::resolveCombat(attacker, defender);

    // Raw damage = max(1, 100 - 0) = 100, but defender only has 3 HP.
    EXPECT_EQ(result.damageToDefender, 3);
    EXPECT_TRUE(result.defenderDied);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
