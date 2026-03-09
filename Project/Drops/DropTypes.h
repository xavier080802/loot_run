#pragma once
#include "PickupPayload.h"

/**
 * @brief Represents a single roll on a loot table (possible item to drop).
 *
 * When an enemy dies, the game goes through each DropEntry in their table
 * and rolls a random number. If the roll succeeds, the item in this entry drops.
 * Each entry defines WHAT can drop, its chance, and if it's a stackable item
 * (like coins or ammo), how many can drop at once.
 */
struct DropEntry
{
    DropType type = DropType::Coin; // What category of item this is (Coin, Ammo, Equipment, etc).
    EquipmentCategory equipmentCategory = EquipmentCategory::None; // If this is an Equipment drop, which category to pull from (or 'Any' for random).
    int itemId = 0; // If equipment, which specific item ID. 0 means "random from the category".
    float chance = 0.0f; // How likely this item is to drop. 1.0 = always drops, 0.5 = 50% chance.
    int minAmount = 0; // Minimum number of coins/ammo/heal units to drop.
    int maxAmount = 0; // Maximum number to drop (the actual amount is randomized between min and max).
};

/**
 * @brief A collection of DropEntry rolls that defines what an enemy can drop.
 *
 * Every enemy type in enemies.json is tied to a loot table via its dropTableId.
 * When the enemy dies, the game looks up their DropTable and runs each entry's
 * random chance roll in sequence.
 *
 * @see GameDB::GetDropTable() - retrieves a DropTable by ID.
 * @see DropSystem::SpawnDrops() - uses this to physically create drop objects.
 */
struct DropTable
{
    int id = 0; // The unique number used to look this table up from GameDB.
    static const int MAX_ENTRIES = 8; // Hard limit on how many different items can be in one table.
    DropEntry entries[MAX_ENTRIES]{}; // The collection of possible drops.
    int entryCount = 0; // How many of the above entries are actually active.
};