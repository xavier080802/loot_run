#include "Camera.h"
#include "Helpers/Vec2Utils.h"
#include "Helpers/MatrixUtils.h"
#include "Helpers/RenderUtils.h"

static AEVec2 translation{};
static float rotation{}, zoom{ 1 };

void ResetCamera(void)
{
	translation = VecZero();
	rotation = zoom = 0;
}

void MoveCamera(f32 deltaX, f32 deltaY)
{
	//+ve delta should make it look like the camera is moving up/right
	//therefore the objects move down/left
	translation.x += deltaX;
	translation.y += deltaY;
}

void SetCameraPos(AEVec2 pos)
{
	translation = pos;
}

void ZoomCamera(f32 deltaZoom)
{
	zoom += deltaZoom;
	zoom = AEMax(0, zoom);
}

void SetCamZoom(f32 _zoom)
{
	zoom = _zoom;
}

void RotateCamera(f32 deltaRot)
{
	rotation += deltaRot;
}

AEVec2 GetCameraTranslation(void)
{
	return translation;
}

float GetCameraZoom()
{
	return zoom;
}

float GetCameraRotation(void)
{
	return rotation;
}

void GetObjViewFromCamera(AEVec2* pos, f32* rot, AEVec2* _scale)
{
	//*_scale = MultVec2(*_scale, scale);
	//AEVec2 ogPos = *pos;
	////Offsets due to rotation and zoom (displacement from camera)
	//pos->x = translation.x + (ogPos.x - translation.x) * AECosDeg(rotation) * scale.x - (ogPos.y - translation.y) * AESinDeg(rotation) * scale.y;
	//pos->y = translation.y + (ogPos.x - translation.x) * AESinDeg(rotation) * scale.x + (ogPos.y - translation.y) * AECosDeg(rotation) * scale.y;
	////Translation due to cam movement
	//*pos = SubtractVec2(*pos, translation);
	//*rot += rotation;

	SetObjViewFromOrigin(pos, rot, _scale, translation, rotation, ToVec2(zoom, zoom));
}
