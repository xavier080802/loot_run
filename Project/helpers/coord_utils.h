//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#ifndef _COORD_UTILS_H_
#define _COORD_UTILS_H_
#include "AEEngine.h"

AEVec2 ScreenToNorm(AEVec2 screenCoords);

AEVec2 WorldToNorm(AEVec2 worldCoords);

AEVec2 ScreenToWorld(AEVec2 screenCoords);

AEVec2 WorldToScreen(AEVec2 worldCoords);
#endif // !_COORD_UTILS_H_
