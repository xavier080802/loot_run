#include "StatsCalc.h"

/**
 * @brief Combines all sources of stats (base, gear, upgrades) into the player or enemy's final real stats.
 *
 * This is called every time the player equips or unequips something (via Player::RecalculateStats).
 * Think of it like pressing "apply" on a character sheet (before this runs, the stats aren't accurate).
 * After it runs, the actor's mStats are ready to use for movement, combat, etc.
 *
 * @param base  The actor's "naked" stats before any gear is applied (e.g., HP = 100).
 *              Passed by const reference because we NEVER want to modify the base stats.
 *              They represent a clean starting point. Pass-by-reference also avoids a copy of the struct.
 *
 * @param equip All the flat bonus numbers added by equipped items (e.g., Sword of Fire adds +20 attack).
 *              Passed by const reference for the same reason (gear data is read-only here).
 *
 * @param upgrades Multipliers applied on top of the sum of base + gear (e.g., x1.5 HP from a potion upgrade).
 *              Passed by const reference (same logic, read-only, no copy needed).
 *
 * @return A newly computed ActorStats struct with the final, real stats ready to use in gameplay.
 *         Returned by VALUE because it's a small struct and we want a fresh copy each recalculation.
 *
 * @note Called by:
 *   - Player::RecalculateStats() - runs any time the player changes their gear
 *   - Player::InitPlayerRuntime() - runs once at game start to set up initial stats
 */
ActorStats StatsCalc::ComputeFinalStats(const ActorStats& base,
    const EquipmentModifiers& equip,
    const UpgradeMultipliers& upgrades)
{
    ActorStats out;

    // Upgrade multipliers scale BASE stats only. Equipment flat bonuses are added on top.
    // Formula: (base * upgradeMultiplier) + equipmentBonus
    // Example: base HP 100, x1.3 upgrade, +50 gear => (100*1.3)+50 = 180
    out.maxHP = (base.maxHP * upgrades.hpMult) + equip.additive.maxHP;
    out.attack = (base.attack * upgrades.attackMult) + equip.additive.attack;
    out.defense = (base.defense * upgrades.defenseMult) + equip.additive.defense;
    out.moveSpeed = (base.moveSpeed * upgrades.moveSpeedMult) + equip.additive.moveSpeed;
    out.attackSpeed = (base.attackSpeed * upgrades.attackSpeedMult) + equip.additive.attackSpeed;

    return out;
}

/**
 * @brief Figures out how much damage actually lands after the target's defense reduces it.
 *
 * Why this formula: the higher the defense, the less damage gets through.
 * But defense can NEVER fully block all damage (you can't reduce to 0 with finite defense).
 *
 * Formula: finalDamage = attack * (100 / (100 + defense))
 * Example: attack = 50, defense = 100 -> 50 * (100/200) = 25 actual damage (defense halved it!)
 *
 * @param attack  The raw damage number being dealt. Passed by VALUE because it's just a float.
 *                No need for a reference to a type this small.
 *
 * @param defense The target's current defense stat. Passed by VALUE for the same reason.
 *
 * @return The actual damage amount that will be subtracted from the target's HP.
 *
 * @note Called by:
 *   - Actor::TakeDamage() - whenever a PHYSICAL or MAGICAL hit lands on any actor.
 *     (Elemental and True Damage skip this function and bypass defense entirely.)
 */
float StatsCalc::ComputeDamage(float attack, float defense)
{
    return attack * (100.0f / (100.0f + defense));
}
