#include "Inventory.h"

/**
 * @brief Helper: checks if an item is a weapon (not null and slot is Weapon).
 * Used internally to make sure only slot weapons into weapon slots.
 */
static bool IsWeapon(const EquipmentData* e)
{
    return e && e->slot == EquipSlot::Weapon;
}

/**
 * @brief Helper: checks if an item is armor (not null and slot is Armor).
 * Used internally to make sure only slot armor into armor slots.
 */
static bool IsArmor(const EquipmentData* e)
{
    return e && e->slot == EquipSlot::Armor;
}

/**
 * @brief Helper: adds all the stat values of 'b' on top of 'a'.
 *
 * This is just a quick utility to merge two stat blocks together
 * without having to write out every field by hand every time.
 *
 * @param a  The stat block we're adding into (modified in-place). Passed by REFERENCE.
 *           we directly change this struct.
 * @param b  The stat block we're reading the bonuses FROM. Passed by CONST REFERENCE.
 *           we only read it, never change it.
 */
static void AddMods(ActorStats& a, const ActorStats& b)
{
    a.maxHP += b.maxHP;
    a.attack += b.attack;
    a.defense += b.defense;
    a.moveSpeed += b.moveSpeed;
    a.attackSpeed += b.attackSpeed;
}

/**
 * @brief Resets the entire inventory back to a blank slate.
 *
 * Wipes all equipped items, removes the weapons in all slots, sets ammo to 0,
 * and clears any upgrade multipliers. Use this when starting a new run or
 * respawning the player from scratch.
 *
 * @note Called by:
 *   - Player::InitPlayerRuntime() - called at the very start of a game session to
 *     ensure no old data lingers from a previous session.
 */
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

/**
 * @brief Stores an item into the inventory's internal item list.
 *
 * Think of this as physically putting the item into the player's backpack.
 * It does NOT equip it, it just records it as owned.
 * Returns false if the inventory is already full.
 *
 * @param data  The item to add. Passed as a CONST POINTER because we only store
 *              a reference to the data, never own or modify the item definition itself.
 *              The actual item data lives in GameDB.
 *
 * @note Called by:
 *   - Player::TryPickup() - when a player walks over a dropped item.
 *   - Player::InitPlayerRuntime() - for starter equipment.
 */
bool Inventory::AddEquipment(const EquipmentData* data)
{
    if (!data) return false;
    if (mEquipCount >= MAX_EQUIP) return false;
    mEquip[mEquipCount++] = data;
    return true;
}

/**
 * @brief Swaps an old item in the inventory for a new one, then equips the new one.
 *
 * Used specifically when the player swaps out a dropped item for the one they're holding.
 * We need to update both the "owned items" list and the active equipped slot.
 *
 * @param oldItem  The item to replace (the one the player is currently wearing).
 *                 Passed as CONST POINTER. Use it to find the slot, not modify it.
 * @param newItem  The item to equip in its place (the one on the ground).
 *                 Passed as CONST POINTER. Store its address, don't change the data.
 *
 * @note Called by:
 *   - Player::Update() - when the player presses [E] to swap their gear with a dropped item.
 */
void Inventory::ReplaceEquipment(const EquipmentData* oldItem, const EquipmentData* newItem)
{
    if (!oldItem || !newItem) return;

    // Find the old item in the physical inventory and swap it
    for (int i = 0; i < mEquipCount; ++i)
    {
        if (mEquip[i] == oldItem)
        {
            mEquip[i] = newItem;
            break;
        }
    }

    // Pass it to the standard equip router so it binds to the weapon1/head/body slot correctly
    Equip(newItem); 
}

/**
 * @brief Smart equip: figures out what type of item this is and slots it properly.
 *
 * Rather than always going to slot 0 or blindly equipping, this checks the item type
 * and sends it to the right sub-function (EquipArmor, EquipBow, or EquipMainWeapon).
 * If both melee weapon slots are already taken, it overwrites slot 0.
 *
 * @param data  The item to equip. Passed as CONST POINTER. Just need its properties.
 *
 * @note Called by:
 *   - Inventory::ReplaceEquipment() - as the second step after updating the item list.
 */
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

/**
 * @brief Puts a specific melee weapon directly into weapon slot 0 or slot 1.
 *
 * This is a direct slot assignment, no auto-routing. Specify exactly where you want
 * the weapon to go. Slot 0 = "weapon1" (primary), Slot 1 = "weapon2" (secondary).
 * Bows cannot be assigned here, they have their own slot.
 *
 * @param slotIndex  Which weapon slot to put the weapon in (0 or 1). Passed by VALUE.
 *                   It's just an integer, cheap to copy.
 * @param data       The weapon to equip. Passed as CONST POINTER. Just stored, not owned.
 *
 * @note Called by:
 *   - Player::InitPlayerRuntime() - to load starter weapons from inventory.json.
 *   - Inventory::Equip() - as a fallback if auto-equip finds no empty slot.
 */
bool Inventory::EquipMainWeapon(int slotIndex, const EquipmentData* data)
{
    if (slotIndex != 0 && slotIndex != 1) return false;

    if (!IsWeapon(data)) return false;
    if (data->weaponType == WeaponType::Bow) return false;

    if (slotIndex == 0) mWeapon1 = data;
    else mWeapon2 = data;

    return true;
}

/**
 * @brief Removes the weapon from the specified melee slot (0 or 1), freeing that slot to empty.
 *
 * Does not remove the item from the inventory list. It just removes it from the active slot.
 * The item can still be re-equipped later if needed.
 *
 * @param slotIndex  Which weapon slot to clear. Passed by VALUE.
 *
 * @note Called by:
 *   - Player::Update() when the player drops their held weapon with the [G] key.
 */
void Inventory::UnequipMainWeapon(int slotIndex)
{
    if (slotIndex == 0) mWeapon1 = nullptr;
    else if (slotIndex == 1) mWeapon2 = nullptr;
}

/**
 * @brief Slots the given weapon into the dedicated bow slot.
 *
 * The bow has its own separate holster from the two melee slots.
 * The weapon must be classified as a Bow (WeaponType::Bow = true) or it'll be rejected.
 *
 * @param data  The bow to equip. Passed as CONST POINTER.
 *
 * @note Called by:
 *   - Player::InitPlayerRuntime() - to load a starter bow from inventory.json.
 *   - Inventory::Equip() - automatically routes bow-type weapons here.
 */
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

/**
 * @brief Slots an armor piece into the correct body slot based on its armorSlot property.
 *
 * The item's armorSlot field (Head, Body, Hands, Feet) decides where it goes.
 * If the slot type doesn't match any known slot, the equip fails.
 *
 * @param data  The armor to equip. Passed as CONST POINTER.
 *
 * @note Called by:
 *   - Player::InitPlayerRuntime() - for starter armor from inventory.json.
 *   - Inventory::Equip() - when an armor item is detected.
 */
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

/**
 * @brief Flips between weapon slot 0 and slot 1 when the player swaps weapons.
 *
 * Simple toggle: if we're on slot 0, switch to 1. If we're on 1, switch to 0.
 * This doesn't change what's in the slots, it just changes which one is "active".
 *
 * @note Called by:
 *   - Player::SubscriptionAlert() when Right Mouse Button is pressed.
 */
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

/**
 * @brief Returns the armor piece in a given slot (Head, Body, Hands, or Feet).
 *
 * Returns nullptr if nothing is equipped in that slot, so callers should null-check.
 * The return is CONST, so callers should only read the armor's stats, never modify them.
 *
 * @param slot  Which armor slot to check. Passed by VALUE.
 *
 * @note Called by:
 *   - Player::DrawUI() - to display what armor is in each slot on the HUD.
 *   - Inventory::GetEquipmentModifiers() - to tally up armor stat bonuses.
 */
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

/**
 * @brief Fills an empty weapon slot with the given weapon, but only if there's room.
 *
 * Goes through weapon1 and weapon2 in order. If one is empty, it claims that slot and stops.
 * If both slots are taken, it does nothing. This avoids overwriting an existing weapon.
 *
 * @param data  The weapon to try equipping. Passed as CONST POINTER.
 *
 * @note Called by:
 *   - Inventory::Equip() - before trying a forced slot assignment.
 */
void Inventory::AutoEquipIfEmpty(const EquipmentData* data)
{
    if (!IsWeapon(data)) return;
    if (data->weaponType == WeaponType::Bow) return;

    if (!mWeapon1) { mWeapon1 = data; return; }
    if (!mWeapon2) { mWeapon2 = data; return; }
}

/**
 * @brief Compiles the total stat bonuses from ALL currently equipped gear.
 *
 * Goes through every equipped item slot (active weapon, bow, and all 4 armor pieces)
 * and adds up all their flat stat bonuses into one combined result.
 * Note: Only the ACTIVE melee weapon contributes. The inactive slot weapon is
 * stored but does NOT add its stats while benched.
 *
 * @return An EquipmentModifiers struct with all stat additions summed up.
 *         Returned by VALUE, it's a fresh copy computed on the spot each call.
 *
 * @note Called by:
 *   - Player::RecalculateStats() - every time gear changes, this rebuilds the gear half
 *     of the final stat formula before StatsCalc::ComputeFinalStats runs.
 */
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

/**
 * @brief Adds arrows (or other ranged ammo) to the player's current stash.
 *
 * The player can hold any amount of ammo. If amount is 0 or negative, it's ignored.
 *
 * @param amount  How many arrows to add. Passed by VALUE.
 *
 * @note Called by:
 *   - Player::TryPickup() - when the player picks up an Ammo drop from an enemy.
 *   - Player::InitPlayerRuntime() - to give the player their starter ammo from inventory.json.
 */
void Inventory::AddAmmo(int amount)
{
    if (amount <= 0) return;
    mAmmo += amount;
}

/**
 * @brief Tries to spend ammo. Returns true if successful, false if not enough.
 *
 * If the player doesn't have enough arrows to fire, this returns false and
 * nothing is consumed. The caller (usually the bow attack logic) should
 * check this return value to decide whether to actually fire.
 *
 * @param amount  The number of arrows to spend. Passed by VALUE.
 *
 * @return true if ammo was deducted, false if the player didn't have enough.
 *
 * @note Called by:
 *   - Player::Update() / bow attack logic to gate shot firing behind "do I have arrows?".
 */
bool Inventory::ConsumeAmmo(int amount)
{
    if (amount <= 0) return true;
    if (mAmmo < amount) return false;
    mAmmo -= amount;
    return true;
}

int Inventory::GetAmmo() const { return mAmmo; }

/**
 * @brief Adds coins to the player's wallet.
 *
 * No upper limit, the player can stack as many coins as they want (for now).
 * Negative or zero amounts are ignored.
 *
 * @param amount  How many coins to add. Passed by VALUE.
 *
 * @note Called by:
 *   - Player::TryPickup() - when the player picks up a Coin drop.
 *   - Player::Update() - when the player sells an item with [C].
 */
void Inventory::AddCoins(int amount)
{
    if (amount <= 0) return;
    mCoins += amount;
}

int Inventory::GetCoins() const
{
    return mCoins;
}
