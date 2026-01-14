#ifndef _CAMERA_H_
#define _CAMERA_H_
#include "AEEngine.h"

/// <summary>
/// Resets camera to default transformation
/// </summary>
void ResetCamera();

/// <summary>
/// +ve: Camera moves up/right
/// </summary>
/// <param name="deltaX">Change in x position of the camera. +ve moves cam right-ward</param>
/// <param name="deltaY">Change in y pos of the cam. +ve moves cam upward</param>
void MoveCamera(f32 deltaX, f32 deltaY);

void SetCameraPos(AEVec2 pos);

/// <summary>
/// +ve values zoom IN.
/// -ve values zoom OUT.
/// </summary>
/// <param name="deltaZoom">Change in zoom</param>
void ZoomCamera(f32 deltaZoom);

/// <summary>
/// +ve values rotate the camera clockwise,
/// -ve values rotate anti-clockwise.
/// 
/// Visually, objects will look like they are rotating the other way
/// </summary>
/// <param name="deltaRot"></param>
void RotateCamera(f32 deltaRot);

AEVec2 GetCameraTranslation();
AEVec2 GetCameraScale();
float GetCameraZoom();
float GetCameraRotation();

//Modifies the arguments to transform the object based on camera's transform
void GetObjViewFromCamera(AEVec2* pos, f32* rot, AEVec2* scale);
#endif // !_CAMERA_H_

