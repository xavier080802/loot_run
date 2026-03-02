#pragma once
#include "AEEngine.h"
#include "DropTypes.h"

// Manages the creation of loot drops when enemies die or chests are opened.
namespace DropSystem
{
	// Spawns a set of items based on a predefined loot table ID at the given location.
	void SpawnDrops(int dropTableId, const AEVec2& worldPos);
}