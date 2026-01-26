#pragma once
#include "../Inventory/EquipmentTypes.h"

enum class DropType : unsigned
{
	Coin = 0,
	Ammo = 1,
	Equipment = 2,
	Heal = 3
};

struct PickupPayload
{
	DropType type = DropType::Coin;
	int amount = 0;                     // coins/ammo/heal
	const EquipmentData* equipment = 0; // if type==Equipment
};