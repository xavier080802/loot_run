#include "LootChest.h"
#include "AEEngine.h"
#include <array>
#include <random>
#include "../Drops/DropSystem.h"

namespace {
	//Furthest distance from chest that loop can fall to
	const float maxDropDist{ 50.f };

	int RandInt(int min, int max)
	{
		if (max - min + 1 == 0) { //Prevent division by 0 error. min and max have a difference of 1
			return rand() % 2 + min;
		}
		return rand() % (max - min + 1) + min;
	}
}

GameObject* LootChest::Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, COLLISION_LAYER _isInLayer)
{
	GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize, _collideWithLayers, _isInLayer);
	goType = GO_TYPE::LOOT_CHEST;
	return this;
}

void LootChest::OnCollide(CollisionData& other)
{
	if (other.other.GetGOType() == GO_TYPE::PLAYER
		&& AEInputCheckTriggered(AEVK_E)) {
		DropLoot();
	}
}

void LootChest::DropLoot()
{
	//TODO: From loot table, pick what items to drop.
	int num{ RandInt(1,5) }; //Number of items to drop
	for (int i{}; i < num; i++) {
		//Create and scatter items nearby
		int b{}, c{};
		//Pythagoras theorem. Randomise x and y displacement, using ^2 to keep the spawn area a circle.
		do {
			c = RandInt(0, static_cast<int>(maxDropDist));
			b = RandInt(0, static_cast<int>(maxDropDist));
		} while (b * b + c * c > maxDropDist * maxDropDist);

		DropSystem::SpawnDrops(0, { pos.x + b, pos.y + c });
	}

	//Disable self
	isEnabled = false;
}
