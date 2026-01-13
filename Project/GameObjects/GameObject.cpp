#include "GameObject.h"
#include "../Helpers/matrix_utils.h"
#include "../helpers/render_utils.h"
#include "../Helpers/vec2_utils.h"
#include "../rendering_manager.h"
#include "../camera.h"
#include <iostream>

GameObject* GameObject::Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, COLLIDER_SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, COLLISION_LAYER _isInLayers) {
	isEnabled = true;
	pos = _pos;
	scale = _scale;
	z = _z;
	//Collider
	collisionEnabled = true;
	colShape = _colShape;
	colSize = _colSize;
	collisionLayers = _collideWithLayers;
	colliderLayer = _isInLayers;
	if (!renderingData) {
		renderingData = new AnimationData;
	}
	renderingData->Init(_meshShape);
	GameObjectManager::GetInstance()->RegisterGO(this);
	return this;
}

void GameObject::Update(double dt)
{
	renderingData->UpdateAnimation(dt);
	renderingData->tint = CreateColor(0, 0, 0, 0);
}

void GameObject::Draw()
{
	f32 _rot = rotationDeg;
	AEVec2 _pos = pos;
	AEVec2 _scale = scale;
	GetObjViewFromCamera(&_pos, &_rot, &_scale);
	//NOTE: Rotation renders ACW
	DrawMeshWithTexOffset(GetTransformMtx(_pos, _rot, _scale),
		renderingData->GetMesh(),
		renderingData->GetTexture(),
		renderingData->tint, renderingData->alpha,
		renderingData->GetTexOffset());
}

AnimationData& GameObject::GetRenderData()
{
	return *renderingData;
}

AEVec2& GameObject::GetPos()
{
	return pos;
}

AEVec2& GameObject::GetColSize()
{
	return colSize;
}

void GameObject::SetCollision(bool enabled)
{
	collisionEnabled = enabled;
}

void GameObject::Free() {
	if (renderingData) {
		renderingData->Free();
		delete renderingData;
		renderingData = nullptr;
	}
}

void GameObject::OnCollide(CollisionData& other)
{
	//Silence "unused param" warning.
	(void)other;

	renderingData->tint = CreateColor(0, 255, 0, 255);
}

void GameObject::CompleteClone(GameObject* const clone)
{
	//Silence "unused param" warning.
	//Can't make this function abstract since GameObject class shouldn't be abstract.
	(void)clone;
}
