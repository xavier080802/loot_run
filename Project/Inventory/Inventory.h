#pragma once
#include "../Actor/StatsTypes.h"
#include "EquipmentTypes.h"

class Inventory
{
public:
    static const int MAX_EQUIP = 16;

    void Clear();

    bool AddEquipment(const EquipmentData* data);

    // --- Equipping ---
    bool Equip(const EquipmentData* data);
    bool EquipMainWeapon(int slotIndex, const EquipmentData* data);
	void UnequipMainWeapon(int slotIndex);
    bool EquipBow(const EquipmentData* data);
	void UnequipBow();

    // Armor equip by sub-slot
    bool EquipArmor(const EquipmentData* data); // uses data->armorSlot

    void SwapMainWeapon();
    const EquipmentData* GetActiveMainWeapon() const;
    const EquipmentData* GetMainWeapon(int slotIndex) const;
    const EquipmentData* GetBow() const;

    // Armor getters
    const EquipmentData* GetArmor(ArmorSlot slot) const;

    void AutoEquipIfEmpty(const EquipmentData* data);

    EquipmentModifiers GetEquipmentModifiers() const;

    void SetUpgradeMultipliers(const UpgradeMultipliers& u);
    UpgradeMultipliers GetUpgradeMultipliers() const;

    void AddAmmo(int amount);
    bool ConsumeAmmo(int amount);
    int GetAmmo() const;

private:
    const EquipmentData* mEquip[MAX_EQUIP]{};
    int mEquipCount = 0;

    const EquipmentData* mWeapon1 = nullptr;
    const EquipmentData* mWeapon2 = nullptr;
    const EquipmentData* mBow = nullptr;
    int mActiveWeaponIndex = 0;

    // Armor slots
    const EquipmentData* mHead = nullptr;
    const EquipmentData* mBody = nullptr;
    const EquipmentData* mHands = nullptr;
    const EquipmentData* mFeet = nullptr;

    UpgradeMultipliers mUpgrades{};
    int mAmmo = 0;
};
