//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#ifndef _COLLISION_H_
#define _COLLISION_H_
#include "AEEngine.h"

bool IsPointOver(float posX, float posY, float width, float height, float pointX, float pointY);
//Check if cursor is over an area in world coordinate system
//pos in world coords
//inHUD: set to false to follow camera
bool IsCursorOverWorld(AEVec2 pos, float width, float height, bool inHUD = true);
bool IsCursorOverWorld(AEVec2 pos, AEVec2 size, bool inHUD = true);
//pos in screen coords
bool IsCursorOverScreen(AEVec2 pos, AEVec2 size);

bool IsPointOverCircle(float posX, float posY, float diameter, float pointX, float pointY);
bool IsCursorOverCircleWorld(AEVec2 pos, float diameter, bool inHUD = true);
//Check if cursor is over an area in world coordinate system
//pos in world coords
//inHUD: set to false to follow camera
bool IsCursorOverOvalWorld(AEVec2 pos, AEVec2 size, f32 rotDeg, bool inHUD = true);
bool IsCursorOverOvalScreen(AEVec2 pos, AEVec2 size, f32 rotDeg);

/// <summary>
/// Returns whether the point is within a ROTATED rect of the given width and height at pos.
/// </summary>
/// <returns>1 or 0</returns>
bool IsPointOverRectRot(float posX, float posY, float width, float height, float degrees, float pointX, float pointY);

/// <summary>
/// Returns whether the cursor is within a ROTATED rect of the given width and height at pos.
/// </summary>
/// <returns>1 or 0</returns>
bool IsCursorOverRectRotWorld(AEVec2 pos, float width, float height, float degrees);

bool IsRectsOverlapping(
	AEVec2 rect1Pos, float rect1Width, float rect1Height,
	AEVec2 rect2Pos, float rect2Width, float rect2Height);

/// <summary>
/// Does not do ovals
/// </summary>
/// <returns>1 if collision is true</returns>
bool CircleRectCollision(AEVec2 rectPos, AEVec2 rectSize, AEVec2 circlePos, float circRad);

bool CircleCollision(AEVec2 pos1, AEVec2 pos2, float radius1, float radius2);

#endif // _COLLISION_H_