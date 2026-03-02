#include "GameObject.h"
#include "../Helpers/MatrixUtils.h"
#include "../helpers/RenderUtils.h"
#include "../Helpers/Vec2Utils.h"
#include "../RenderingManager.h"
#include "../Camera.h"
#include <iostream>

GameObject::GameObject(bool _register)
{
	if (_register)
		GameObjectManager::GetInstance()->RegisterGO(this);
}

GameObject* GameObject::Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, Collision::LAYER _isInLayer) {
	isEnabled = true;
	pos = _pos;
	scale = _scale;
	z = _z;
	velocity = initialVel = { 0,0 };
	//Collider
	collisionEnabled = true;
	colShape = _colShape;
	colSize = _colSize;
	collisionLayers = _collideWithLayers;
	colliderLayer = _isInLayer;
	if (!renderingData) {
		renderingData = new AnimationData;
	}
	renderingData->Init(_meshShape);
	renderingData->tint = CreateColor(255, 255, 255, 255);
	GameObjectManager::GetInstance()->RegisterGO(this);
	return this;
}

void GameObject::Update(double dt)
{
	renderingData->UpdateAnimation(dt);
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

bool GameObject::CanCollide() const
{
	return isEnabled && collisionEnabled;
}

Bitmask GameObject::GetCollisionLayers() const
{
	return collisionLayers;
}

Collision::LAYER GameObject::GetColliderLayer() const
{
	return colliderLayer;
}

GO_TYPE GameObject::GetGOType() const
{
	return goType;
}

bool GameObject::IsEnabled() const
{
	return isEnabled;
}

void GameObject::SetEnabled(bool enable)
{
	isEnabled = enable;
}

void GameObject::SetPos(AEVec2 nextPos)
{
	pos = nextPos;
}

void GameObject::Move(AEVec2 moveAmt)
{
	SetPos(pos + moveAmt);
}

void GameObject::SetCollision(bool enabled)
{
	collisionEnabled = enabled;
}

void GameObject::SetCollisionLayers(Bitmask layers)
{
	collisionLayers = layers;
}

void GameObject::SetColliderLayer(Collision::LAYER layer)
{
	colliderLayer = layer;
}

void GameObject::SetZ(int _z)
{
	z = _z;
}

void GameObject::ApplyForce(AEVec2 force)
{
	//Reset vel to prevent it from compounding and breaking. Really should not tho, but ehh
	initialVel = velocity = VecZero();
	velocity += force;
	//To taper off velocity linearly, need the initial vel
	//So the velocity falls off after about a second.
	initialVel += force; 
}

bool GameObject::HasForceApplied() const
{
	return velocity.x || velocity.y;
}

AEVec2 const& GameObject::GetVelocity() const
{
	return velocity;
}

void GameObject::Temp_DoVelocityMovement(double dt)
{
	if (!HasForceApplied()) return;
	//Testing: External force
	Move(velocity * dt); 

	//Decay velocity
	AEVec2 prevVel = velocity;
	//Initial vel for a more linear decay
	velocity.x = (abs(velocity.x) <= 1.f ? 0 : velocity.x - initialVel.x * dt);
	velocity.y = (abs(velocity.y) <= 1.f ? 0 : velocity.y - initialVel.y * dt);

	//Clamping to 0 if moveAmt pushes velocity to other direction.
	if ((prevVel.x < 0 && velocity.x > 0) || (prevVel.x > 0 && velocity.x < 0)) velocity.x = 0;
	if (prevVel.y < 0 && velocity.y > 0 || (prevVel.y > 0 && velocity.y < 0)) velocity.y = 0;

	if (AEVec2Length(&velocity) <= 1.f) {
		velocity = initialVel = VecZero();
	}
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
}

void GameObject::OnCollideTile(std::pair<TileMap::Tile const&, AEVec2> tile)
{
	(void)tile; //Silence unused param warning
}

void GameObject::CompleteClone(GameObject* const clone)
{
	//Silence "unused param" warning.
	//Can't make this function abstract since GameObject class shouldn't be abstract.
	(void)clone;
}
