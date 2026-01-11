#include "camera.h"
#include "Helpers/vec2_utils.h"
#include "Helpers/matrix_utils.h"
#include "Helpers/render_utils.h"

static AEVec2 translation, scale{ 1,1 };
static float rotation;

void ResetCamera(void)
{
	translation = VecZero();
	scale = VecOne();
	rotation = 0;
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
	AEVec2 pre = scale;
	scale.x += deltaZoom;
	scale.y += deltaZoom;
	scale.x = AEMax(0, scale.x);
	scale.y = AEMax(0, scale.y);
}

void RotateCamera(f32 deltaRot)
{
	rotation += deltaRot;
}

AEVec2 GetCameraTranslation(void)
{
	return translation;
}

AEVec2 GetCameraZoom(void)
{
	return scale;
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

	SetObjViewFromOrigin(pos, rot, _scale, translation, rotation, scale);
}
