#include "StatsCalc.h"

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

float StatsCalc::ComputeDamage(float attack, float defense)
{
    return attack * (100.0f / (100.0f + defense));
}