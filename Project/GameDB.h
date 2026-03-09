#pragma once

#include "./Drops/DropTypes.h"
#include "./Inventory/EquipmentTypes.h"
#include "./Actor/EnemyDef.h"

namespace GameDB
{
    // Retrieves a predefined loot table by ID
    const DropTable* GetDropTable(int id);
    bool LoadDropTables(const char* path);

    // Enemies
    // Parses and loads enemy definitions from a file into the database
    bool LoadEnemyDefs(const char* path);
    
    // Retrieves a specific enemy's immutable definition by ID
    const EnemyDef* GetEnemyDef(int id);

    // Equipment
    bool LoadEquipmentDefs(const char* path, EquipmentCategory category);
    // Retrieves immutable stats and definitions for a specific piece of equipment
    const EquipmentData* GetEquipmentData(EquipmentCategory category, int id);
    const EquipmentData* GetRandomEquipment();

    void UnloadEquipmentReg();

    struct PlayerInventoryDef {
        int weapon1 = 0;
        int weapon2 = 0;
        int bow = 0;
        int head = 0;
        int body = 0;
        int hands = 0;
        int feet = 0;
        int ammo = 0;
    };

    // Player Configuration
    bool LoadPlayerDef(const char* path);
    bool LoadPlayerInventory(const char* path);
    const ActorStats& GetPlayerBaseStats();
    const PlayerInventoryDef& GetPlayerStarterInventory();

    // runtime owned flags not used yet ignore
    void InitEquipmentRuntime();
    bool IsOwned(int id);
    void SetOwned(int id, bool owned);
}
