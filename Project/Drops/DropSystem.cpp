#include "DropSystem.h"
#include "../GameDB.h"
#include "PickupGO.h"
#include <cstdlib> // rand

static float Rand01()
{
    return (float)rand() / (float)RAND_MAX;
}

static int RandRangeInt(int lo, int hi)
{
    if (hi < lo) { int t = lo; lo = hi; hi = t; }
    if (hi == lo) return lo;
    return lo + (rand() % (hi - lo + 1));
}

void DropSystem::SpawnDrops(int dropTableId, const AEVec2& worldPos)
{
    //const DropTable* table = GameDB::GetDropTable(dropTableId);
    //if (!table) return;

    //for (int i = 0; i < table->entryCount; ++i)
    //{
    //    const DropEntry& e = table->entries[i];
    //    if (e.chance <= 0.0f) continue;
    //    if (Rand01() > e.chance) continue;

    //    PickupPayload payload;
    //    payload.type = e.type;

    //    if (e.type == DropType::Equipment)
    //    {
    //        payload.amount = 1;
    //        payload.equipment = GameDB::GetEquipmentData(e.itemId);
    //        if (!payload.equipment) continue;
    //    }
    //    else
    //    {
    //        payload.amount = RandRangeInt(e.minAmount, e.maxAmount);
    //        payload.equipment = 0;
    //        if (payload.amount <= 0) continue;
    //    }

    //    // Spawn pickup GO (uses manager registration via GameObject::Init)
    //    PickupGO::Spawn(worldPos, payload);
    //}
}