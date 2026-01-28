#include "Inventory.h"

void Inventory::Clear()
{
    mEquipCount = 0;
    for (int i = 0; i < MAX_EQUIP; ++i) mEquip[i] = 0;
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

EquipmentModifiers Inventory::GetEquipmentModifiers() const
{
    EquipmentModifiers out;

    for (int i = 0; i < mEquipCount; ++i)
    {
        const EquipmentData* e = mEquip[i];
        if (!e) continue;

        out.additive.maxHP += e->mods.additive.maxHP;
        out.additive.attack += e->mods.additive.attack;
        out.additive.defense += e->mods.additive.defense;
        out.additive.moveSpeed += e->mods.additive.moveSpeed;
        out.additive.attackSpeed += e->mods.additive.attackSpeed;
    }

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