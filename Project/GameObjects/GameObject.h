#ifndef _GAME_OBJECT_H_
#define _GAME_OBJECT_H_
#include "AEEngine.h"
#include "../Helpers/CollisionUtils.h"
#include "../Helpers/BitmaskUtils.h"
#include "../Rendering/AnimationData.h"
#include "GameObjectManager.h"	
#include "../Map.h"

class GameObject {
	friend class GameObjectManager; //Allow manager access to private members.
public:
	enum COLLISION_LAYER {
		NONE,
		PLAYER,
		ENEMIES,
		OBSTACLE,
		INTERACTABLE,
		PET,
	};
	//Making this a struct so we can extend easier
	struct CollisionData {
		GameObject& other;
	};
	
	//Set values and register to the manager. Call only once per GO
	//Need to manually call init.
	//Ps. set goType for derived game objects
	virtual GameObject* Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, COLLISION_LAYER _isInLayer);
	virtual void Update(double dt);
	virtual void Draw();
	virtual void Free();
	virtual void OnCollide(CollisionData& other);

	AnimationData& GetRenderData();
	const AEVec2& GetPos() const;
	const AEVec2& GetScale() const;
	const AEVec2& GetColSize() const;
	int GetZ() const;
	bool CanCollide() const;
	Bitmask GetCollisionLayers()const;
	COLLISION_LAYER GetColliderLayer()const;
	virtual GO_TYPE GetGOType() const;

	void SetPos(AEVec2 nextPos);
	void Move(AEVec2 moveAmt);
	void SetCollision(bool enabled);
	//Set what layers this GO can collide with.
	void SetCollisionLayers(Bitmask layers);
	//Set what layer this GO is considered in.
	void SetColliderLayer(COLLISION_LAYER layer);

	//Apply a force (directional) on this go.
	void ApplyForce(AEVec2 force);
	bool HasForceApplied() const;
	AEVec2 const& GetVelocity() const;

	//TEMP
	void Temp_DoVelocityMovement(double dt);

	std::vector<unsigned> cellIndexes{};

protected:
	/// For GameObject derivatives (derived from GameObject) to clone properly.
	/// Any members that are pointers must be new'd for the clone.
	/// Do not touch members that are in the GameObject class.
	virtual void CompleteClone(GameObject*const clone);

	AEVec2 pos{}, scale{ 1,1 };
	int z{}; //Rendering order. Higher = rendered above an obj with lower z
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

	AEVec2 velocity{}, prevPos{}, initialVel{};

	GO_TYPE goType{};
};
#endif // !_GAME_OBJECT_H_
