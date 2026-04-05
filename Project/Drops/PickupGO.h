#ifndef PICKUPGO_H_
#define PICKUPGO_H_

#include "AEEngine.h"
#include "../GameObjects/GameObject.h"
#include "PickupPayload.h"

// Represents a physical item dropped in the game world that a player can pick up.
class PickupGO : public GameObject
{
public:
	// Factory method to instantiate a new drop at a specific world position
	static PickupGO* Spawn(const AEVec2& worldPos, const PickupPayload& payload);

	// Initializes the GameObject with a specific payload (what the item actually is)
	// Must be called after GameObject::Init()
	void InitPickup(const PickupPayload& payload);

	void Update(double dt) override;
	
	// Handles intersection logic, usually triggering TryPickup on the Player
	// void OnCollide(CollisionData& other) override;

	// Retrieves what this pickup contains
	const PickupPayload& GetPayload() const { return mPayload; }

private:
	PickupPayload mPayload{}; // The actual item data (Ammo, Health, Equipment, etc.)
};

#endif // PICKUPGO_H_

