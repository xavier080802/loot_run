#pragma once

#include "./Drops/DropTypes.h"
#include "./Inventory/EquipmentTypes.h"
#include "./Actor/EnemyDef.h"

namespace GameDB
{
    const DropTable* GetDropTable(int id);

    // Enemies
    bool LoadEnemyDefs(const char* path);
    const EnemyDef* GetEnemyDef(int id);

    // Equipment
    const EquipmentData* GetEquipmentData(int id);

    // runtime owned flags not used yet ignore
    void InitEquipmentRuntime();
    bool IsOwned(int id);
    void SetOwned(int id, bool owned);
}
