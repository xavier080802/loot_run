#include "StatsCalc.h"

ActorStats StatsCalc::ComputeFinalStats(const ActorStats& base,
    const EquipmentModifiers& equip,
    const UpgradeMultipliers& upgrades)
{
    ActorStats out;

    out.maxHP = (base.maxHP + equip.additive.maxHP) * upgrades.hpMult;
    out.attack = (base.attack + equip.additive.attack) * upgrades.attackMult;
    out.defense = (base.defense + equip.additive.defense) * upgrades.defenseMult;

    // movement/attack speed currently additive
    out.moveSpeed = base.moveSpeed + equip.additive.moveSpeed;
    out.attackSpeed = base.attackSpeed + equip.additive.attackSpeed;

    return out;
}

float StatsCalc::ComputeDamage(float attack, float defense)
{
    return attack * (100.0f / (100.0f + defense));
}