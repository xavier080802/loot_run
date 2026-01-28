#pragma once
#include "../Actor/StatsTypes.h"
#include "EquipmentTypes.h"

class Inventory
{
public:
	static const int MAX_EQUIP = 16;

	void Clear();

	bool AddEquipment(const EquipmentData* data);
	EquipmentModifiers GetEquipmentModifiers() const;

	void SetUpgradeMultipliers(const UpgradeMultipliers& u);
	UpgradeMultipliers GetUpgradeMultipliers() const;

	void AddAmmo(int amount);
	bool ConsumeAmmo(int amount);
	int GetAmmo() const;

private:
	const EquipmentData* mEquip[MAX_EQUIP]{};
	int mEquipCount = 0;

	UpgradeMultipliers mUpgrades{};
	int mAmmo = 0;
};