//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#ifndef _MATRIX_UTILS_H_
#define _MATRIX_UTILS_H_
#include "AEEngine.h"

AEMtx33 GetTransformMtx(AEVec2 translation, f32 rotationDeg, AEVec2 scaling);

void GetTransformMtx(AEMtx33& mtx, AEVec2 translation, f32 rotationDeg, AEVec2 scaling);
#endif // !_MATRIX_UTILS_H_
