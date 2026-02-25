#pragma once

#include "./Drops/DropTypes.h"
#include "./Inventory/EquipmentTypes.h"
#include "./Actor/EnemyDef.h"

namespace GameDB
{
    // Retrieves a predefined loot table by ID
    const DropTable* GetDropTable(int id);

    // Enemies
    // Parses and loads enemy definitions from a file into the database
    bool LoadEnemyDefs(const char* path);
    
    // Retrieves a specific enemy's immutable definition by ID
    const EnemyDef* GetEnemyDef(int id);

    // Equipment
    // Retrieves immutable stats and definitions for a specific piece of equipment
    const EquipmentData* GetEquipmentData(int id);

    // runtime owned flags not used yet ignore
    void InitEquipmentRuntime();
    bool IsOwned(int id);
    void SetOwned(int id, bool owned);
}
