#ifndef _GAME_OBJECT_H_
#define _GAME_OBJECT_H_
#include "AEEngine.h"
#include "../Helpers/collision.h"
#include "../Helpers/bitmask_utils.h"
#include "../render_data.h"

class GameObject {
public:
	virtual GameObject* Init(AEVec2 _pos, AEVec2 _scale, COLLIDER_SHAPE _colShape, AEVec2 _colSize, Bitmask _colLayers);
	virtual void Update();
	virtual void Draw();

	void SetCollision(bool enabled);

	enum COLLISION_LAYER {
		NONE,
		PLAYER,
		ENEMIES,
		OBSTACLE
	};

protected:
	AEVec2 pos, scale;
	float rotationDeg;
	bool isActive;
	bool collisionEnabled;
	COLLIDER_SHAPE colShape = COLLIDER_SHAPE::CIRCLE;
	AEVec2 colSize;
	Bitmask collisionLayers;
	RenderData* renderingData;
};
#endif // !_GAME_OBJECT_H_
