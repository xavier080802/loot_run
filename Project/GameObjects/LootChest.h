#ifndef _LOOT_CHEST_H_
#define _LOOT_CHEST_H_
#include "GameObject.h"

class LootChest : public GameObject
{
public:
	GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, COLLISION_LAYER _isInLayer) override;
	void OnCollide(CollisionData& other) override;

private:
	void DropLoot();

	//TODO: Loot table
};

#endif // !_LOOT_CHEST_H_
