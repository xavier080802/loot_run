#include "game_state.h"

void GameState_Init()
{
    GameObjects_Init();
    mainCam.pos = player.pos;
}

// Update player position and camera
void GameState_Update()
{
    AEInputUpdate();

    // Ensure frame controller is running
    static bool started = false;
    if (!started)
    {
        AEFrameRateControllerStart();
        started = true;
    }

    float deltaTime = AEFrameRateControllerGetFrameTime();
    float speed = 300.0f * deltaTime;

    if (AEInputCheckCurr(AEVK_W)) player.pos.y += speed;
    if (AEInputCheckCurr(AEVK_S)) player.pos.y -= speed;
    if (AEInputCheckCurr(AEVK_A)) player.pos.x -= speed;
    if (AEInputCheckCurr(AEVK_D)) player.pos.x += speed;

    mainCam.pos = player.pos;
    AEGfxSetCamPosition(mainCam.pos.x, mainCam.pos.y);
}


// Draw map and player
void GameState_Draw()
{
    if (!squareMesh || !circleMesh) return; // prevent errors

    // Draw map
    AEMtx33 scale, translate, transform;
    AEMtx33Scale(&scale, map.size.x, map.size.y);
    AEMtx33Trans(&translate, map.pos.x, map.pos.y);
    AEMtx33Concat(&transform, &translate, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // float color
    AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

    // Draw player
    AEMtx33Scale(&scale, player.radius, player.radius);
    AEMtx33Trans(&translate, player.pos.x, player.pos.y);
    AEMtx33Concat(&transform, &translate, &scale);
    AEGfxSetTransform(transform.m);
    AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f); // green
    AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
}

