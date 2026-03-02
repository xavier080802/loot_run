//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#ifndef _COORD_UTILS_H_
#define _COORD_UTILS_H_
#include "AEEngine.h"

//Screen coords to Normalized coords
AEVec2 ScreenToNorm(AEVec2 screenCoords);
//World to normalized coords
AEVec2 WorldToNorm(AEVec2 worldCoords);
//Norm to world
AEVec2 NormToWorld(AEVec2 normCoords);
//Screen to world coords
AEVec2 ScreenToWorld(AEVec2 screenCoords);
//World to screen coords
AEVec2 WorldToScreen(AEVec2 worldCoords);
//Screen coords to World, but camera translation is accounted for.
AEVec2 ScreenToCameraWorld(AEVec2 screenCoords);
#endif // !_COORD_UTILS_H_
