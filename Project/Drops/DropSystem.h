#ifndef DROPSYSTEM_H_
#define DROPSYSTEM_H_

#include "AEEngine.h"
#include "DropTypes.h"

// Manages the creation of loot drops when enemies die or chests are opened.
namespace DropSystem
{
	//Read some data from json
	bool InitDropSystemSettings();

	// Spawns a set of items based on a predefined loot table ID at the given location.
	void SpawnDrops(int dropTableId, const AEVec2& worldPos);

	//Add a text to show what the player picked up
	void AddToPickupDisplay(const PickupPayload& payload);

	//Prints the recent items picked up
	//Call in an update function
	void PrintPickupDisplay();

	void UpdatePickupDisplay(float dt);

	void ClearAllPickups();
}

#endif // DROPSYSTEM_H_

