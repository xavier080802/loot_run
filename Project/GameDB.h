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

    // Returns a random enemy definition from the Normal tier, or nullptr if none exist
    const EnemyDef* GetRandomNormalEnemy();

    // Returns a random enemy definition from the Elite tier, or nullptr if none exist
    const EnemyDef* GetRandomEliteEnemy();

    // Returns a random enemy definition from the Boss tier, or nullptr if none exist
    const EnemyDef* GetRandomBossEnemy();

    /**
     * Rolls a weighted random enemy definition (Normal or Elite only).
     * Boss enemies are excluded; use GetRandomBossEnemy() for explicit boss spawning.
     * Returns nullptr if the roll falls outside both bands or the pool is empty.
     * @param normalChance  Probability [0..1] that a Normal enemy is chosen (default 70%).
     * @param eliteChance   Probability [0..1] that an Elite enemy is chosen (default 30%).
     */
    const EnemyDef* GetWeightedRandomEnemy(float normalChance = 0.70f, float eliteChance = 0.30f);

    // Equipment
    bool LoadEquipmentDefs(const char* path, EQUIPMENT_CATEGORY category);
    // Retrieves immutable stats and definitions for a specific piece of equipment
    const EquipmentData* GetEquipmentData(EQUIPMENT_CATEGORY category, int id);
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
    const char* GetPlayerTexturePath();
    float GetPlayerRadius();

    // runtime owned flags not used yet ignore
    void InitEquipmentRuntime();
    bool IsOwned(int id);
    void SetOwned(int id, bool owned);
}
