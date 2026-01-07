//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#include "collision.h"
#include "../helpers/coord_utils.h"
#include "../helpers/vec2_utils.h"
#include "../helpers/matrix_utils.h"
#include <math.h>

int IsPointOver(float posX, float posY, float width, float height, float pointX, float pointY)
{
	return (pointX >= posX - width / 2 && pointX <= posX + width / 2 && pointY >= posY - height / 2 && pointY <= posY + height / 2) ? 1 : 0;
}

int IsCursorOver(AEVec2 pos, float width, float height)
{
	AEVec2 mouse = GetMouseVec();
	return IsPointOver(pos.x, pos.y, width, height, mouse.x, mouse.y);
}

int IsPointOverCircle(float posX, float posY, float diameter, float pointX, float pointY)
{
	//return (fabs(pointX - posX) <= diameter/2.f && fabs(pointY - posY) <= diameter/2.f) ? 1 : 0;
	return (pointX - posX) * (pointX - posX) + (pointY - posY) * (pointY - posY) <= (diameter / 2.f) * (diameter / 2.f);
}

int IsCursorOverCircle(AEVec2 pos, float diameter)
{
	AEVec2 world = GetMouseVec();
	return (fabs(world.x - pos.x) <= diameter / 2.f && fabs(world.y - pos.y) <= diameter / 2.f) ? 1 : 0;
}

int IsCursorOverOval(AEVec2 pos, AEVec2 size, f32 rotDeg) {
	AEVec2 world = GetMouseVec();

	AEVec2 v = { 0,0 }, r = { 0,0 };
	AEMtx33 A = { 0 };
	AEVec2Set(&v, world.x - pos.x, world.y - pos.y); // point x (mouse), shifted to Origin
	AEMtx33Scale(&A, 1 / (size.x), 1 / (size.y)); //Inverse scaling matrix

	AEMtx33 Ar = { 0 };
	AEMtx33RotDeg(&Ar, rotDeg); //Inverse rot (Rendering is ACW, so using +ve rot here)
	AEMtx33 T = { 0 };
	AEMtx33Concat(&T, &A, &Ar); //Combine. Normally it's TRS, now its SR (T is 0 here)
	AEVec2Set(&r, T.m[0][0] * v.x + T.m[0][1] * v.y, T.m[1][0] * v.x + T.m[1][1] * v.y); //Image of v
	//AEVec2Set(&r, A.m[0][0] * v.x, A.m[1][1] * v.y); //Image of v
	AEVec2 zero = { 0 };
	return IsPointOverCircle(zero.x, zero.y, 1, r.x, r.y); //Inversing the scaling of size (A*size) gives normal circle of d=1
}

/// <summary>
/// Using 3-step approach to compute whether the point is over a rotated rect. (Affline transformation)
/// 1. Shift point, such that pos is the Origin, and the rect is rotated about the Origin. -> Point v
/// 2. Perform the linear transformation: Inverse rotation matrix (A) -> A * v (matrix mult)
/// 3. Profit. (Check if the image of the point is within the rect. Since v is about (0, 0) due to step 1,
/// Using vector_zero instead of shifting everything back).
/// </summary>
int IsPointOverRectRot(float posX, float posY, float width, float height, float degrees, float pointX, float pointY)
{
	AEVec2 v = { 0,0 }, r = { 0,0 };
	AEMtx33 A = { 0 };
	AEVec2Set(&v, pointX - posX, pointY - posY); // point x (mouse), shifted till the rotation is around Origin
	AEMtx33Rot(&A, -AEDegToRad(degrees)); //Inverse Rot matrix
	AEVec2Set(&r, A.m[0][0] * v.x + A.m[0][1] * v.y, A.m[1][0] * v.x + A.m[1][1] * v.y); //Image of v after rot
	AEVec2 zero = { 0 };
	return IsPointOver(zero.x, zero.y, width, height, r.x, r.y);
}

int IsCursorOverRectRot(AEVec2 pos, float width, float height, float degrees)
{
	AEVec2 world = GetMouseVec();
	return IsPointOverRectRot(pos.x, pos.y, width, height, degrees, world.x, world.y);
}

int IsRectsOverlapping(
	AEVec2 rect1Pos, float rect1Width, float rect1Height,
	AEVec2 rect2Pos, float rect2Width, float rect2Height) 
{
	AEVec2 l1 = { 0 }, r1 = { 0 }, l2 = { 0 }, r2 = { 0 };
	AEVec2Set(&l1, rect1Pos.x - rect1Width / 2.f, rect1Pos.y - rect1Height / 2.f);
	AEVec2Set(&r1, rect1Pos.x + rect1Width / 2.f, rect1Pos.y + rect1Height / 2.f);
	AEVec2Set(&l2, rect2Pos.x - rect2Width / 2.f, rect2Pos.y - rect2Height / 2.f);
	AEVec2Set(&r2, rect2Pos.x + rect2Width / 2.f, rect2Pos.y + rect2Height / 2.f);
	return !((l1.x > r2.x || l2.x > r1.x) || (r1.y < l2.y || r2.y < l1.y));
}

int CircleRectCollision(AEVec2 rectPos, AEVec2 rectSize, AEVec2 circlePos, float circRad)
{
	//Vars for which rect edges to test
	float testX = 0, testY = 0;
	if (circlePos.x < rectPos.x - rectSize.x) testX = rectPos.x - rectSize.x; //C on left edge
	else if (circlePos.x < rectPos.x + rectSize.x) testX = rectPos.y + rectSize.x; //C on right edge
	if (circlePos.y < rectPos.y - rectSize.y) testY = rectPos.y - rectSize.y; //C on bottom edge
	else if (circlePos.y < rectPos.y + rectSize.y) testY = rectPos.y + rectSize.y; //C on top edge

	float distX = circlePos.x - testX;
	float distY = circlePos.y - testY;
	float distSqr = distX * distX + distY * distY;
	return distSqr <= circRad * circRad;
}
