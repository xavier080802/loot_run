#pragma once
#include "StatsTypes.h"

namespace StatsCalc
{
    ActorStats ComputeFinalStats(const ActorStats& base,
        const EquipmentModifiers& equip,
        const UpgradeMultipliers& upgrades);

    float ComputeDamage(float attack, float defense);
}