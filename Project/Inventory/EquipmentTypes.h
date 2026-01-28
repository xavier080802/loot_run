#pragma once
#include <cstdint>

#include "../Actor/StatsTypes.h"

enum class EquipSlot : uint8_t
{
    Weapon = 0,
    Armor = 1,
};

enum class WeaponType : uint8_t
{
    None = 0,
    Sword = 1,
    Bow = 2,
};

// How the weapon behaves (your design)
enum class AttackType : uint8_t
{
    None = 0,
    SwingArc = 1,
    Stab = 2,
    Projectile = 3,
    CircleAOE = 4,
};

struct EquipmentData
{
    int id = 0;

    EquipSlot slot = EquipSlot::Weapon;

    // weapon behavior
    WeaponType weaponType = WeaponType::None;
    AttackType attackType = AttackType::None;
    bool isRanged = false;

    // THIS matches Inventory.cpp usage: e->mods.additive.maxHP, etc.
    EquipmentModifiers mods{};

    const char* name = "";
};