#pragma once
#include <cstdint>
#include <string>
#include "../Actor/StatsTypes.h"
#include "../Elements/Element.h"
#include "../Helpers/ColorUtils.h"

enum class EQUIP_SLOT : uint8_t
{
    WEAPON = 0,
    ARMOR = 1,
};

enum class EQUIPMENT_CATEGORY : uint8_t
{
    NONE = 0,
    MELEE = 1,
    RANGED = 2,
    HEAD = 3,
    BODY = 4,
    HANDS = 5,
    FEET = 6,
};

// Armor Slot (only used when slot == Armor)
enum class ARMOR_SLOT : uint8_t
{
    NONE = 0,
    HEAD = 1,
    BODY = 2,
    HANDS = 3,
    FEET = 4,
};

enum class WEAPON_TYPE : uint8_t
{
    NONE = 0,
    SWORD = 1,
    BOW = 2,
    AXE = 3,
    HAMMER = 4,
};

// Add on as needed
enum class ATTACK_TYPE : uint8_t
{
    NONE = 0,
    SWINGARC = 1,
    STAB = 2,
    PROJECTILE = 3,
    CIRCLE_AOE = 4,
};


// Standard rarity tiers for items, potentially affecting drop chance or coloring in UI.
enum class ITEM_RARITY : uint8_t
{
    COMMON = 0,
    UNCOMMON = 1,
    RARE = 2,
    EPIC = 3,
    LEGENDARY = 4,
    MYTHICAL = 5,
    UNIQUE = 6,
};

/// Returns the display tint colour associated with a given rarity tier.
inline Color GetRarityColor(ITEM_RARITY r)
{
    switch (r) {
    case ITEM_RARITY::COMMON:    return Color{ 139, 69, 19, 255 }; // Brown
    case ITEM_RARITY::UNCOMMON:  return Color{ 100, 220, 100, 255 }; // Green
    case ITEM_RARITY::RARE:      return Color{ 100, 150, 255, 255 }; // Blue
    case ITEM_RARITY::EPIC:      return Color{ 180,  80, 255, 255 }; // Purple
    case ITEM_RARITY::LEGENDARY: return Color{ 255, 165,   0, 255 }; // Orange
    case ITEM_RARITY::MYTHICAL:  return Color{ 255, 215,   0, 255 }; // Gold
    case ITEM_RARITY::UNIQUE:    return Color{ 255, 100, 180, 255 }; // Pink
    default:                return Color{ 255, 255, 255, 255 }; // White fallback
    }
}

// Represents the static properties of an equippable item.
// Often instantiated once in a database and referenced by pointer.
struct EquipmentData
{
    int id = 0; // Unique identifier used for database lookups
    EQUIPMENT_CATEGORY category = EQUIPMENT_CATEGORY::NONE; // Used with ID to uniquely identify item

    EQUIP_SLOT slot = EQUIP_SLOT::WEAPON; // Primary type of item (Weapon vs Armor)
    ARMOR_SLOT armorSlot = ARMOR_SLOT::NONE; // Only used when slot == Armor

    // weapon behavior overrides
    WEAPON_TYPE weaponType = WEAPON_TYPE::NONE; // Broad classification of weapon
    ATTACK_TYPE attackType = ATTACK_TYPE::NONE; // Determines the shape/hitbox logic of attacks
    bool isRanged = false; // If true, consumes ammo and fires projectiles
    Elements::ELEMENT_TYPE element = Elements::ELEMENT_TYPE::NONE; // The element applied on hit
    float knockback = 100.0f; // Force applied to target when hit
    float attackSize = 10.0f; // Size of the projectile or hitbox

    // item quality
    ITEM_RARITY rarity = ITEM_RARITY::COMMON;

    // Stat modifiers (additive bonuses granted while equipped)
    EquipmentModifiers mods{};

    int sellPrice = 10; // Amount of coins gained when selling

    const char* name = ""; // Display name
    const char* texturePath = ""; // Path to the image file (defaults to empty/current)
};
