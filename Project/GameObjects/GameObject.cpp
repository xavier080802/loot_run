#include "GameObject.h"
#include "../Helpers/MatrixUtils.h"
#include "../helpers/RenderUtils.h"
#include "../Helpers/Vec2Utils.h"
#include "../RenderingManager.h"
#include "../Camera.h"
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

const AEVec2& GameObject::GetPos() const
{
	return pos;
}

const AEVec2& GameObject::GetScale() const
{
	return scale;
}

const AEVec2& GameObject::GetColSize() const
{
	return colSize;
}

int GameObject::GetZ() const
{
	return z;
}

Bitmask GameObject::GetCollisionLayers() const
{
	return collisionLayers;
}

GameObject::COLLISION_LAYER GameObject::GetColliderLayer() const
{
	return colliderLayer;
}

void GameObject::SetPos(AEVec2 nextPos)
{
	pos = nextPos;
}

void GameObject::SetCollision(bool enabled)
{
	collisionEnabled = enabled;
}

void GameObject::SetCollisionLayers(Bitmask layers)
{
	collisionLayers = layers;
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
