#pragma once
#include "../Inventory/EquipmentTypes.h"

/**
 * @brief The kinds of things that can be picked up from the ground.
 *
 * This enum is used both by the loot table system (to define what can be dropped)
 * and by the pickup objects on the ground (to know what they ARE and what to do
 * when the player picks them up).
 */
enum class DropType : unsigned
{
    Coin = 0,       // Adds to the player's coin count.
    Ammo = 1,       // Adds arrows to the player's ammo stash.
    Equipment = 2,  // A weapon or armor piece the player can equip.
    Heal = 3,       // Restores a portion of the player's HP.
    Buff = 4,       // Applies a status buff (not yet implemented in drops).
    None = 5        // Intentionally drops nothing. Used in loot tables to pad bad luck.
};

/**
 * @brief A bundle of information that describes a dropped item waiting to be picked up.
 *
 * When something dies and its drops are spawned, each physical item on the ground
 * carries one of these payloads so the game knows what to give the player when they walk over it.
 *
 * @note Used by:
 *   - PickupGO - the physical item game object holds one of these to describe itself.
 *   - Player::TryPickup() - reads this to know what to add to the player's inventory.
 *   - DropSystem::SpawnDrops() - fills this in and attaches it to a spawned PickupGO.
 */
struct PickupPayload
{
    DropType type = DropType::Coin; // What kind of pickup this is.
    int amount = 0;                     // For coins, ammo, and heals: how many/much is in this pickup.
    const EquipmentData* equipment = 0; // For Equipment drops: a pointer to the item's static data.
};