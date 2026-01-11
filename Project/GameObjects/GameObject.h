#ifndef _GAME_OBJECT_H_
#define _GAME_OBJECT_H_
#include "AEEngine.h"
#include "../Helpers/collision.h"
#include "../Helpers/bitmask_utils.h"
#include "../animation_data.h"

class GameObject {
public:
	virtual GameObject* Init(AEVec2 _pos, AEVec2 _scale, MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize, Bitmask _colLayers);
	virtual void Update(double dt);
	virtual void Draw();
	virtual void Free();

	AnimationData& GetRenderData();
	void SetCollision(bool enabled);

	enum COLLISION_LAYER {
		NONE,
		PLAYER,
		ENEMIES,
		OBSTACLE
	};

protected:
	AEVec2 pos{}, scale{ 1,1 };
	float rotationDeg{};
	bool isActive{ false };
	bool collisionEnabled{true};
	COLLIDER_SHAPE colShape{ COLLIDER_SHAPE::COL_CIRCLE };
	AEVec2 colSize{};
	Bitmask collisionLayers{};
	AnimationData* renderingData{};
};
#endif // !_GAME_OBJECT_H_
