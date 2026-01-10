#pragma once
#include "AEEngine.h"

struct GameObject
{
    AEVec2 pos;
    float radius;
    AEVec2 size; // for map
};

// Global objects
extern GameObject player;
extern GameObject map;

// Meshes
extern AEGfxVertexList* circleMesh;
extern AEGfxVertexList* squareMesh;

// Initialization
void GameObjects_Init();
