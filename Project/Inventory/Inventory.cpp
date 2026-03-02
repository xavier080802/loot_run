#include "Inventory.h"

static bool IsWeapon(const EquipmentData* e)
{
    return e && e->slot == EquipSlot::Weapon;
}

static bool IsArmor(const EquipmentData* e)
{
    return e && e->slot == EquipSlot::Armor;
}

static void AddMods(ActorStats& a, const ActorStats& b)
{
    a.maxHP += b.maxHP;
    a.attack += b.attack;
    a.defense += b.defense;
    a.moveSpeed += b.moveSpeed;
    a.attackSpeed += b.attackSpeed;
}

void Inventory::Clear()
{
    mEquipCount = 0;
    for (int i = 0; i < MAX_EQUIP; ++i) mEquip[i] = nullptr;

    mWeapon1 = nullptr;
    mWeapon2 = nullptr;
    mBow = nullptr;
    mActiveWeaponIndex = 0;

    mHead = mBody = mHands = mFeet = nullptr;

    mAmmo = 0;
    mUpgrades = UpgradeMultipliers{};
}

bool Inventory::AddEquipment(const EquipmentData* data)
{
    if (!data) return false;
    if (mEquipCount >= MAX_EQUIP) return false;
    mEquip[mEquipCount++] = data;
    return true;
}

bool Inventory::Equip(const EquipmentData* data)
{
    if (!data) return false;

    if (data->slot == EquipSlot::Armor)
        return EquipArmor(data);

    // Weapon routing
    if (data->weaponType == WeaponType::Bow)
        return EquipBow(data);

    // If it's a weapon (but not a bow), we first try to auto-equip it if slots are empty
    AutoEquipIfEmpty(data);

    // If it successfully auto-equipped into weapon1 or weapon2, we're done
    if (mWeapon1 == data || mWeapon2 == data) return true;

    // Otherwise, force it into slot 0 (replacing current weapon1)
    return EquipMainWeapon(0, data);
}

bool Inventory::EquipMainWeapon(int slotIndex, const EquipmentData* data)
{
    if (slotIndex != 0 && slotIndex != 1) return false;

    if (!IsWeapon(data)) return false;
    if (data->weaponType == WeaponType::Bow) return false;

    if (slotIndex == 0) mWeapon1 = data;
    else mWeapon2 = data;

    return true;
}

void Inventory::UnequipMainWeapon(int slotIndex)
{
    if (slotIndex == 0) mWeapon1 = nullptr;
    else if (slotIndex == 1) mWeapon2 = nullptr;
}

bool Inventory::EquipBow(const EquipmentData* data)
{
    if (!IsWeapon(data)) return false;
    if (data->weaponType != WeaponType::Bow) return false;

    mBow = data;
    return true;
}

void Inventory::UnequipBow()
{
    mBow = nullptr;
}

bool Inventory::EquipArmor(const EquipmentData* data)
{
    if (!IsArmor(data)) return false;

    switch (data->armorSlot)
    {
    case ArmorSlot::Head:  mHead = data;  return true;
    case ArmorSlot::Body:  mBody = data;  return true;
    case ArmorSlot::Hands: mHands = data; return true;
    case ArmorSlot::Feet:  mFeet = data;  return true;
    default:
        return false;
    }
}

void Inventory::SwapMainWeapon()
{
    mActiveWeaponIndex = (mActiveWeaponIndex == 0) ? 1 : 0;
}

const EquipmentData* Inventory::GetActiveMainWeapon() const
{
    return (mActiveWeaponIndex == 0) ? mWeapon1 : mWeapon2;
}

const EquipmentData* Inventory::GetMainWeapon(int slotIndex) const
{
    if (slotIndex == 0) return mWeapon1;
    if (slotIndex == 1) return mWeapon2;
    return nullptr;
}

const EquipmentData* Inventory::GetBow() const
{
    return mBow;
}

const EquipmentData* Inventory::GetArmor(ArmorSlot slot) const
{
    switch (slot)
    {
    case ArmorSlot::Head:  return mHead;
    case ArmorSlot::Body:  return mBody;
    case ArmorSlot::Hands: return mHands;
    case ArmorSlot::Feet:  return mFeet;
    default: return nullptr;
    }
}

void Inventory::AutoEquipIfEmpty(const EquipmentData* data)
{
    if (!IsWeapon(data)) return;
    if (data->weaponType == WeaponType::Bow) return;

    if (!mWeapon1) { mWeapon1 = data; return; }
    if (!mWeapon2) { mWeapon2 = data; return; }
}

EquipmentModifiers Inventory::GetEquipmentModifiers() const
{
    EquipmentModifiers out;

    // Tally up stats from equipped weapons
    // Only get stats from the actively held melee weapon and the bow.
    const EquipmentData* activeMelee = GetActiveMainWeapon();
    if (activeMelee) AddMods(out.additive, activeMelee->mods.additive);
    if (mBow)        AddMods(out.additive, mBow->mods.additive);

    // Tally up stats from equipped armor slots
    if (mHead)  AddMods(out.additive, mHead->mods.additive);
    if (mBody)  AddMods(out.additive, mBody->mods.additive);
    if (mHands) AddMods(out.additive, mHands->mods.additive);
    if (mFeet)  AddMods(out.additive, mFeet->mods.additive);

    return out;
}

void Inventory::SetUpgradeMultipliers(const UpgradeMultipliers& u) { mUpgrades = u; }
UpgradeMultipliers Inventory::GetUpgradeMultipliers() const { return mUpgrades; }

void Inventory::AddAmmo(int amount)
{
    if (amount <= 0) return;
    mAmmo += amount;
}

bool Inventory::ConsumeAmmo(int amount)
{
    if (amount <= 0) return true;
    if (mAmmo < amount) return false;
    mAmmo -= amount;
    return true;
}

int Inventory::GetAmmo() const { return mAmmo; }
