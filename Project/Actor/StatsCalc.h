#pragma once
#include "StatsTypes.h"

namespace StatsCalc
{
    // Merges base stats with equipment additions and upgrade multipliers
    // to produce the final effective stats for an actor at runtime.
    ActorStats ComputeFinalStats(const ActorStats& base,
        const EquipmentModifiers& equip,
        const UpgradeMultipliers& upgrades);

    // Determines the final amount of health to deduct from an attack
    // after factoring in the target's defense capabilities.
    float ComputeDamage(float attack, float defense);
}