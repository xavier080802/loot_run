//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#ifndef _COLLISION_H_
#define _COLLISION_H_
#include "AEEngine.h"

enum COLLIDER_SHAPE {
	COL_CIRCLE,
	COL_RECT,
};

bool IsPointOver(float posX, float posY, float width, float height, float pointX, float pointY);
bool IsCursorOver(AEVec2 pos, float width, float height);

bool IsPointOverCircle(float posX, float posY, float diameter, float pointX, float pointY);
bool IsCursorOverCircle(AEVec2 pos, float diameter);
bool IsCursorOverOval(AEVec2 pos, AEVec2 size, f32 rotDeg);

/// <summary>
/// Returns whether the point is within a ROTATED rect of the given width and height at pos.
/// </summary>
/// <returns>1 or 0</returns>
bool IsPointOverRectRot(float posX, float posY, float width, float height, float degrees, float pointX, float pointY);

/// <summary>
/// Returns whether the cursor is within a ROTATED rect of the given width and height at pos.
/// </summary>
/// <returns>1 or 0</returns>
bool IsCursorOverRectRot(AEVec2 pos, float width, float height, float degrees);

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