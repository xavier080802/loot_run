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

//Initialize static
std::forward_list<LootChestOpenedSub*> LootChest::openedChestSubs_static{};

GameObject* LootChest::Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, Collision::LAYER _isInLayer)
{
	GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize, _collideWithLayers, _isInLayer);
	goType = GO_TYPE::LOOT_CHEST;
	GetRenderData().AddTexture("Assets/chest.png");
	return this;
}

void LootChest::OnCollide(CollisionData& other)
{
	if (other.other.GetGOType() == GO_TYPE::PLAYER
		&& AEInputCheckTriggered(AEVK_E)) {
		DropLoot();

		//Global alert (TEMP: Arg content is placeholder)
		AlertChestOpenedSubs({ 0 });
	}
}

//Static
void LootChest::SubToChestOpened(LootChestOpenedSub* sub)
{
	//Check dupe
	for (LootChestOpenedSub* s : openedChestSubs_static) {
		if (s == sub) return;
	}
	openedChestSubs_static.push_front(sub);
}

//Static
void LootChest::UnsubFromChestOpened(LootChestOpenedSub* sub)
{
	//Find and remove subscriber from list.
	for (LootChestOpenedSub* s : openedChestSubs_static) {
		if (s != sub) continue;
		openedChestSubs_static.remove(s);
		break;
	}
}

//Static
void LootChest::AlertChestOpenedSubs(LootChestSubContent const& content)
{
	for (LootChestOpenedSub* s : openedChestSubs_static) {
		s->SubscriptionAlert(content);
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
