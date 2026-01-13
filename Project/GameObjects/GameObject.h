#ifndef _GAME_OBJECT_H_
#define _GAME_OBJECT_H_
#include "AEEngine.h"
#include "../Helpers/collision.h"
#include "../Helpers/bitmask_utils.h"
#include "../animation_data.h"
#include "GameObjectManager.h"	

class GameObject {
	friend class GameObjectManager; //Allow manager access to private members.
public:
	enum COLLISION_LAYER {
		NONE,
		PLAYER,
		ENEMIES,
		OBSTACLE
	};
	//Making this a struct so we can extend easier
	struct CollisionData {
		GameObject& other;
	};
	
	//Set values and register to the manager. Call only once per GO
	virtual GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, COLLISION_LAYER _isInLayers);
	virtual void Update(double dt);
	virtual void Draw();
	virtual void Free();
	virtual void OnCollide(CollisionData& other);

	AnimationData& GetRenderData();
	AEVec2& GetPos();
	AEVec2& GetColSize();
	void SetCollision(bool enabled);
	std::vector<unsigned> cellIndexes{};

protected:
	/// For GameObject derivatives (derived from GameObject) to clone properly.
	/// Any members that are pointers must be new'd for the clone.
	/// Do not touch members that are in the GameObject class.
	virtual void CompleteClone(GameObject*const clone);

	AEVec2 pos{}, scale{ 1,1 };
	int z{};
	float rotationDeg{};
	bool isEnabled{ false };
	bool collisionEnabled{true};
	COLLIDER_SHAPE colShape{ COLLIDER_SHAPE::COL_CIRCLE };
	AEVec2 colSize{};
	//Layers to collide with
	Bitmask collisionLayers{};
	//Layer that this GO is in
	COLLISION_LAYER colliderLayer{};
	AnimationData* renderingData{};
};
#endif // !_GAME_OBJECT_H_
