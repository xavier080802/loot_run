#include "LootChest.h"
#include "AEEngine.h"
#include <array>
#include <random>
#include "../Drops/DropSystem.h"
#include "../GameDB.h"

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

LootChest& LootChest::SetDropTable(int id)
{
	dropTable = GameDB::GetDropTable(id);
	return *this;
}

GameObject* LootChest::Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, Collision::LAYER _isInLayer)
{
	GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize, _collideWithLayers, _isInLayer);
	goType = GO_TYPE::LOOT_CHEST;
	GetRenderData().AddTexture("Assets/chest.png");
	SetDropTable(rand() % 2);
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
	if (dropTable) {
		DropSystem::SpawnDrops(dropTable->id, pos);
	}
	//Disable self
	isEnabled = false;
}
