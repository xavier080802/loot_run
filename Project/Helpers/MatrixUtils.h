#ifndef MATRIXUTILS_H_
#define MATRIXUTILS_H_

//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#include "AEEngine.h"

AEMtx33 GetTransformMtx(AEVec2 translation, f32 rotationDeg, AEVec2 scaling);

void GetTransformMtx(AEMtx33& mtx, AEVec2 translation, f32 rotationDeg, AEVec2 scaling);

#endif // MATRIXUTILS_H_

