#include "game_object.h"
#include <cmath>

GameObject player;
GameObject map;

AEGfxVertexList* circleMesh = nullptr;
AEGfxVertexList* squareMesh = nullptr;

void GameObjects_Init()
{
    // Player circle
    AEGfxMeshStart();
    const int segments = 36;
    const float step = 360.0f / segments;
    for (int i = 0; i < segments; ++i)
    {
        float a0 = AEDegToRad(i * step);
        float a1 = AEDegToRad((i + 1) * step);
        float c0x = cosf(a0), c0y = sinf(a0);
        float c1x = cosf(a1), c1y = sinf(a1);
        AEGfxTriAdd(
            0.0f, 0.0f, 0xFFFFFFFF, 0.5f, 0.5f,
            c0x, c0y, 0xFFFFFFFF, 0.5f + c0x * 0.5f, 0.5f + c0y * 0.5f,
            c1x, c1y, 0xFFFFFFFF, 0.5f + c1x * 0.5f, 0.5f + c1y * 0.5f);
    }
    circleMesh = AEGfxMeshEnd();  // must check if null

    // Map square
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFF444444, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFF444444, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFF444444, 1.0f, 0.0f);
    AEGfxTriAdd(-0.5f, -0.5f, 0xFF444444, 0.0f, 1.0f,
        0.5f, 0.5f, 0xFF444444, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFF444444, 0.0f, 0.0f);
    squareMesh = AEGfxMeshEnd();  // must check if null

    // Initialize player and map
    player.pos = { 0.0f, 0.0f };
    player.radius = 15.0f; // smaller

    map.pos = { 0.0f, 0.0f };
    map.size = { 2000.0f, 2000.0f }; // bigger
}

