#pragma once
#include <cstdint>
#include "../Actor/StatsTypes.h"

// Category of equipment
enum class EquipSlot : uint8_t
{
    Weapon = 0,
    Armor = 1,
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


enum class Rarity : uint8_t
{
    Common = 0,
    Uncommon = 1,
    Rare = 2,
    Epic = 3,
    Legendary = 4,
};

struct EquipmentData
{
    int id = 0;

    EquipSlot slot = EquipSlot::Weapon;
    ArmorSlot armorSlot = ArmorSlot::None; // Only used when slot == Armor

    // weapon behavior
    WeaponType weaponType = WeaponType::None;
    AttackType attackType = AttackType::None;
    bool isRanged = false;

    // item quality
    Rarity rarity = Rarity::Common;

    // Stat modifiers (additive)
    EquipmentModifiers mods{};

    const char* name = "";
};
