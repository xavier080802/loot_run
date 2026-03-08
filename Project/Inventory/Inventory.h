#pragma once
#include "../Actor/StatsTypes.h"
#include "EquipmentTypes.h"

// Manages the player's collection of items, equipped gear, and current ammo.
class Inventory
{
public:
    static const int MAX_EQUIP = 16; // Maximum number of items the inventory can hold

    // Empties the inventory of all items, un-equips everything, and resets ammo.
    void Clear();

    // Attempts to add an item to the inventory. Returns true if successful (space available).
    bool AddEquipment(const EquipmentData* data);

    // Attempts to automatically equip the given item based on its slot type.
    bool Equip(const EquipmentData* data);
    
    // Finds oldItem in the inventory and replaces its pointer with newItem, then equips newItem.
    void ReplaceEquipment(const EquipmentData* oldItem, const EquipmentData* newItem);
    
    // Forces the equipment into a specific main weapon slot (0 or 1).
    bool EquipMainWeapon(int slotIndex, const EquipmentData* data);
    
    // Removes the weapon from the specified slot.
	void UnequipMainWeapon(int slotIndex);
	
	// Equips the provided weapon specifically into the Bow slot.
    bool EquipBow(const EquipmentData* data);
	void UnequipBow();

    // Armor equip by sub-slot (determines slot via data->armorSlot)
    bool EquipArmor(const EquipmentData* data);

    // Toggles the active weapon index between 0 and 1.
    void SwapMainWeapon();
    
    // Returns the currently active melee weapon based on mActiveWeaponIndex.
    const EquipmentData* GetActiveMainWeapon() const;
    const EquipmentData* GetMainWeapon(int slotIndex) const;
    const EquipmentData* GetBow() const;

    // Armor getters
    const EquipmentData* GetArmor(ArmorSlot slot) const;

    // Equips the item only if its corresponding slot is currently empty.
    void AutoEquipIfEmpty(const EquipmentData* data);

    // Compiles the stat bonuses from all currently equipped items (Weapons + Armor).
    EquipmentModifiers GetEquipmentModifiers() const;

    void SetUpgradeMultipliers(const UpgradeMultipliers& u);
    UpgradeMultipliers GetUpgradeMultipliers() const;

    void AddAmmo(int amount);
    // Attempts to reduce ammo by 'amount'. Returns true if successful, false if not enough ammo.
    bool ConsumeAmmo(int amount);
    int GetAmmo() const;

    void AddCoins(int amount);
    int GetCoins() const;

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
    int mCoins = 0;
};
