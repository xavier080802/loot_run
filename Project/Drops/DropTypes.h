#pragma once
#include "PickupPayload.h"

struct DropEntry
{
	DropType type = DropType::Coin;
	int itemId = 0;      // equipmentId if Equipment, else 0
	float chance = 0.0f; // 0..1
	int minAmount = 0;
	int maxAmount = 0;
};

struct DropTable
{
	int id = 0;
	static const int MAX_ENTRIES = 8;
	DropEntry entries[MAX_ENTRIES]{};
	int entryCount = 0;
};