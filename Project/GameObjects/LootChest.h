#ifndef _LOOT_CHEST_H_
#define _LOOT_CHEST_H_
#include "GameObject.h"
#include "../DesignPatterns/Subscriber.h"
#include <forward_list>

struct LootChestSubContent;
struct LootChestOpenedSub;

class LootChest : public GameObject
{
public:
	GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, Collision::LAYER _isInLayer) override;
	void OnCollide(CollisionData& other) override;

	//Global: subscriber pattern stuff
	static void SubToChestOpened(LootChestOpenedSub* sub);
	static void UnsubFromChestOpened(LootChestOpenedSub* sub);
	static void AlertChestOpenedSubs(LootChestSubContent const& content);

private:
	static std::forward_list<LootChestOpenedSub*> openedChestSubs_static;

	void DropLoot();

	//TODO: Loot table
};

//------------------------------------------------------------------

//Alert when loot chest is opened
struct LootChestSubContent {
	int dropTableId;
};
struct LootChestOpenedSub : Subscriber<LootChestSubContent> {
	void SubscriptionAlert(LootChestSubContent content) override = 0;
};

#endif // !_LOOT_CHEST_H_
