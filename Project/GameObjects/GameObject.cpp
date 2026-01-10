#include "GameObject.h"
#include "../Helpers/matrix_utils.h"
#include "../helpers/render_utils.h"
#include "../Helpers/vec2_utils.h"
#include "../rendering_manager.h"
#include "../camera.h"

GameObject* GameObject::Init(AEVec2 _pos, AEVec2 _scale, COLLIDER_SHAPE _colShape, AEVec2 _colSize, Bitmask _colLayers) {
	isActive = true;
	pos = _pos;
	scale = _scale;
	//Collider
	collisionEnabled = true;
	colShape = _colShape;
	colSize = _colSize;
	collisionLayers = _colLayers;
	return this;
}

void GameObject::Update()
{
}

void GameObject::Draw()
{
	f32 _rot = rotationDeg;
	AEVec2 _pos = pos;
	AEVec2 _scale = scale;
	GetObjViewFromCamera(&_pos, &_rot, &_scale);
	//NOTE: Rotation renders ACW
	DrawMeshWithTexOffset(GetTransformMtx(_pos, _rot, _scale),
		RenderingManager::GetInstance()->GetMesh(renderingData->meshShape),
		renderingData->GetTexture(),
		renderingData->tint, renderingData->alpha,
		renderingData->GetTexOffset());
}

void GameObject::SetCollision(bool enabled)
{
	collisionEnabled = enabled;
}
