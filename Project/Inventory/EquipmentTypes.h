#pragma once
#include <cstdint>
#include <string>
#include "../Actor/StatsTypes.h"
#include "../Elements/Element.h"

enum class EquipSlot : uint8_t
{
    Weapon = 0,
    Armor = 1,
};

enum class EquipmentCategory : uint8_t
{
    None = 0,
    Melee = 1,
    Ranged = 2,
    Head = 3,
    Body = 4,
    Hands = 5,
    Feet = 6,
};

// Armor Slot (only used when slot == Armor)
enum class ArmorSlot : uint8_t
{
    None = 0,
    Head = 1,
    Body = 2,
    Hands = 3,
    Feet = 4,
};

enum class WeaponType : uint8_t
{
    None = 0,
    Sword = 1,
    Bow = 2,
};

// Add on as needed
enum class AttackType : uint8_t
{
    None = 0,
    SwingArc = 1,
    Stab = 2,
    Projectile = 3,
    CircleAOE = 4,
};


// Standard rarity tiers for items, potentially affecting drop chance or coloring in UI.
enum class Rarity : uint8_t
{
    Common = 0,
    Uncommon = 1,
    Rare = 2,
    Epic = 3,
    Legendary = 4,
    Mythical = 5,
    Unique = 6,
};

// Represents the static properties of an equippable item.
// Often instantiated once in a database and referenced by pointer.
struct EquipmentData
{
    int id = 0; // Unique identifier used for database lookups
    EquipmentCategory category = EquipmentCategory::None; // Used with ID to uniquely identify item

    EquipSlot slot = EquipSlot::Weapon; // Primary type of item (Weapon vs Armor)
    ArmorSlot armorSlot = ArmorSlot::None; // Only used when slot == Armor

    // weapon behavior overrides
    WeaponType weaponType = WeaponType::None; // Broad classification of weapon
    AttackType attackType = AttackType::None; // Determines the shape/hitbox logic of attacks
    bool isRanged = false; // If true, consumes ammo and fires projectiles
    Elements::ELEMENT_TYPE element = Elements::ELEMENT_TYPE::NONE; // The element applied on hit
    float knockback = 100.0f; // Force applied to target when hit
    float attackSize = 10.0f; // Size of the projectile or hitbox

    // item quality
    Rarity rarity = Rarity::Common;

    // Stat modifiers (additive bonuses granted while equipped)
    EquipmentModifiers mods{};

    int sellPrice = 10; // Amount of coins gained when selling

    const char* name = ""; // Display name
    const char* texturePath = ""; // Path to the image file (defaults to empty/current)
};
