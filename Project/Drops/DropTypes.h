#pragma once
#include "PickupPayload.h"

// Represents a single possible item that can be dropped by an entity.
struct DropEntry
{
	DropType type = DropType::Coin;
	EquipmentCategory equipmentCategory = EquipmentCategory::None; // Used if Equipment
	int itemId = 0; // equipmentId if Equipment, else 0
	float chance = 0.0f; // Probability this item will drop (0.0 to 1.0)
	int minAmount = 0; // Minimum quantity dropped if successful
	int maxAmount = 0; // Maximum quantity dropped if successful
};

// Represents a collection of items (a loot table) that an entity can drop upon death.
struct DropTable
{
	int id = 0; // Identifier to look up this table in the game database
	static const int MAX_ENTRIES = 8;
	DropEntry entries[MAX_ENTRIES]{};
	int entryCount = 0; // The actual number of active entries in this table
};