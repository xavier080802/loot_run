//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#ifndef _VEC2_UTILS_H_
#define _VEC2_UTILS_H_
#include "AEEngine.h"

AEVec2 ToVec2(f32 x, f32 y);

AEVec2 VecZero(void);
AEVec2 VecOne(void);

//Returns the mouse screen coords, converted to world space
AEVec2 GetMouseVec(void);

//Inverses the values of v
AEVec2 NegVec2(AEVec2 v);

/// <summary>
/// Multiplies the vectors
/// 
/// (a.x * b.y, a.y * b.y)
/// </summary>
AEVec2 MultVec2(AEVec2 a, AEVec2 b);

//Compares if 2 vecs are equal
bool CompareVec2(AEVec2 const& a, AEVec2 const& b);

AEVec2 operator-(AEVec2 lhs, AEVec2 rhs);
AEVec2 operator-=(AEVec2& lhs, AEVec2 rhs);

AEVec2 operator+(AEVec2 lhs, AEVec2 rhs);
AEVec2 operator+=(AEVec2& lhs, AEVec2 rhs);
#endif // !_VEC2_UTILS_H_
