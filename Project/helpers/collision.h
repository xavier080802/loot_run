//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#ifndef _COLLISION_H_
#define _COLLISION_H_
#include "AEEngine.h"

int IsPointOver(float posX, float posY, float width, float height, float pointX, float pointY);
int IsCursorOver(AEVec2 pos, float width, float height);

int IsPointOverCircle(float posX, float posY, float diameter, float pointX, float pointY);
int IsCursorOverCircle(AEVec2 pos, float diameter);
int IsCursorOverOval(AEVec2 pos, AEVec2 size, f32 rotDeg);

/// <summary>
/// Returns whether the point is within a ROTATED rect of the given width and height at pos.
/// </summary>
/// <returns>1 or 0</returns>
int IsPointOverRectRot(float posX, float posY, float width, float height, float degrees, float pointX, float pointY);

/// <summary>
/// Returns whether the cursor is within a ROTATED rect of the given width and height at pos.
/// </summary>
/// <returns>1 or 0</returns>
int IsCursorOverRectRot(AEVec2 pos, float width, float height, float degrees);

int IsRectsOverlapping(
	AEVec2 rect1Pos, float rect1Width, float rect1Height,
	AEVec2 rect2Pos, float rect2Width, float rect2Height);

/// <summary>
/// Does not do ovals
/// </summary>
/// <returns>1 if collision is true</returns>
int CircleRectCollision(AEVec2 rectPos, AEVec2 rectSize, AEVec2 circlePos, float circRad);

#endif // _COLLISION_H_