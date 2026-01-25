#pragma once
#include "AEEngine.h"
#include "../GameObjects/GameObject.h"
#include "PickupPayload.h"

class PickupGO : public GameObject
{
public:
	static PickupGO* Spawn(const AEVec2& worldPos, const PickupPayload& payload);

	// Call after Init()
	void InitPickup(const PickupPayload& payload);

	void Update(double dt) override;
	void OnCollide(CollisionData& other) override;

	const PickupPayload& GetPayload() const { return mPayload; }

private:
	PickupPayload mPayload{};
};