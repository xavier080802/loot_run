#include "game_state.h"
#include "../Music.h"
#include "../helpers/render_utils.h"
#include "../helpers/matrix_utils.h"
#include "../helpers/vec2_utils.h"
#include "../helpers/coord_utils.h"
#include "../camera.h"
#include "../rendering_manager.h"
#include "../GameObjects/GameObject.h"
#include <iostream>

namespace {
    // Music
    BGMManager bgm;

    // Meshes for rendering
    AEGfxVertexList* circleMesh = nullptr;
    AEGfxVertexList* borderMesh = nullptr;

    // Player state
    AEVec2 playerPos;
    AEVec2 playerDir = { 1.0f, 0.0f };   // last non-zero movement direction
    float playerRadius = 15.0f;
    float playerSpeed = 300.0f;

    // Camera state
    AEVec2 camPos;
    AEVec2 camVel;
    float camZoom = 1.0f;   // default zoom
    float camSmoothTime = 0.25f;  // smoothing factor
    float softMargin = 100.0f; // easing near edges

    // Map dimensions
    float mapWidth = 2000.0f;
    float mapHeight = 2000.0f;
    float halfMapWidth;
    float halfMapHeight;

    // Minimap settings
    float minimapWidth = 200.0f;
    float minimapHeight = 200.0f;
    float minimapMargin = 20.0f;
    float minimapPlayerScaleFactor = 2.0f; // make player dot bigger

    // Minimap arrow settings
    float minimapArrowLengthPx = 10.0f;
    float minimapArrowThicknessPx = 8.0f;
    float minimapArrowExtraOffsetPx = 8.0f;
    unsigned int minimapArrowColor = 0xFF00FFFF; // cyan

    // --------------------
    // Small helpers
    // --------------------
    inline float Saturate(float x) { return AEClamp(x, 0.0f, 1.0f); }
    inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

    // Build a unit square border mesh (center at origin, size = 1)
    AEGfxVertexList* CreateBorderRectMesh(void) {
        AEGfxMeshStart();
        AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
        AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
        AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
        AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
        AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
        return AEGfxMeshEnd();
    }

    void UpdateWorldMap(float dt) {
        // --- Camera follow with soft easing near edges ---
        float winW = (float)AEGfxGetWinMaxX();
        float winH = (float)AEGfxGetWinMaxY();
        float halfViewW = (winW * 0.5f) / camZoom;
        float halfViewH = (winH * 0.5f) / camZoom;

        float minX = -halfMapWidth + halfViewW;
        float maxX = halfMapWidth - halfViewW;
        float minY = -halfMapHeight + halfViewH;
        float maxY = halfMapHeight - halfViewH;

        // If viewport is larger than map, lock camera to center
        if (minX > maxX) minX = maxX = 0.0f;
        if (minY > maxY) minY = maxY = 0.0f;

        AEVec2 camTarget = playerPos;

        // Soft easing factors near edges
        float tLeft = 1.0f - Saturate((playerPos.x - (minX + softMargin)) / softMargin);
        float tRight = 1.0f - Saturate(((maxX - softMargin) - playerPos.x) / softMargin);
        float tBottom = 1.0f - Saturate((playerPos.y - (minY + softMargin)) / softMargin);
        float tTop = 1.0f - Saturate(((maxY - softMargin) - playerPos.y) / softMargin);

        camTarget.x = Lerp(playerPos.x, minX, tLeft);
        camTarget.x = Lerp(camTarget.x, maxX, tRight);
        camTarget.y = Lerp(playerPos.y, minY, tBottom);
        camTarget.y = Lerp(camTarget.y, maxY, tTop);

        // SmoothDamp camera movement
        float omega = 2.0f / camSmoothTime;
        float xd = omega * dt;
        float expK = 1.0f / (1.0f + xd + 0.48f * xd * xd + 0.235f * xd * xd * xd);

        AEVec2 change = { camPos.x - camTarget.x, camPos.y - camTarget.y };
        AEVec2 temp = { (camVel.x + omega * change.x) * dt,
                          (camVel.y + omega * change.y) * dt };

        camVel.x = (camVel.x - omega * temp.x) * expK;
        camVel.y = (camVel.y - omega * temp.y) * expK;

        camPos.x = camTarget.x + (change.x + temp.x) * expK;
        camPos.y = camTarget.y + (change.y + temp.y) * expK;

        // Clamp camera inside map bounds
        camPos.x = AEClamp(camPos.x, minX, maxX);
        camPos.y = AEClamp(camPos.y, minY, maxY);

        // Clamp zoom
        camZoom = AEClamp(camZoom, 0.5f, 2.5f);

        //Gameobject rendering relies on camera.h values, so syncing here.
        SetCameraPos(camPos); 
        SetCamZoom(camZoom);
    }

    void DrawMinimapArrow(float mmX, float mmY, float scaleX, float scaleY)
    {
        // Player position in minimap space
        float cx = mmX + playerPos.x * scaleX;
        float cy = mmY + playerPos.y * scaleY;

        // Direction vector in minimap space 
        float dx = playerDir.x * scaleX;
        float dy = playerDir.y * scaleY;
        float mag = sqrtf(dx * dx + dy * dy);
        if (mag < 1e-4f) { dx = 1.0f; dy = 0.0f; mag = 1.0f; }
        dx /= mag; dy /= mag;

        // Perpendicular vector (for arrow base width)
        float px = -dy;
        float py = dx;

        // Offset the arrow away from the player dot
        float dotRadius = playerRadius * 0.5f * (scaleX + scaleY) * minimapPlayerScaleFactor;
        float offset = dotRadius + minimapArrowExtraOffsetPx;
        float ax = cx + dx * offset;
        float ay = cy + dy * offset;

        // Arrow dimensions
        float length = minimapArrowLengthPx;
        float thickness = minimapArrowThicknessPx;
        float baseBack = length * 0.2f;

        // Arrow points
        float tipX = ax + dx * length;
        float tipY = ay + dy * length;
        float baseLeftX = ax - dx * baseBack + px * (thickness * 0.5f);
        float baseLeftY = ay - dy * baseBack + py * (thickness * 0.5f);
        float baseRightX = ax - dx * baseBack - px * (thickness * 0.5f);
        float baseRightY = ay - dy * baseBack - py * (thickness * 0.5f);

        // Build and draw the triangle
        AEGfxMeshStart();
        AEGfxTriAdd(tipX, tipY, minimapArrowColor, 0, 0,
            baseLeftX, baseLeftY, minimapArrowColor, 0, 0,
            baseRightX, baseRightY, minimapArrowColor, 0, 0);
        AEGfxVertexList* arrowMesh = AEGfxMeshEnd();

        AEMtx33 identity;
        AEMtx33Identity(&identity);
        AEGfxSetTransform(identity.m);

        AEGfxMeshDraw(arrowMesh, AE_GFX_MDM_TRIANGLES);
        if (arrowMesh) AEGfxMeshFree(arrowMesh);
    }

    void RenderWorldMap(void) {
        AEGfxStart();
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // --------------------
        // Camera transform
        // --------------------
        AEMtx33 camMatrix;
        //AEMtx33Scale(&camScale, camZoom, camZoom);
        //AEMtx33Trans(&camTrans, -camPos.x, -camPos.y);
        //AEMtx33Concat(&camMatrix, &camTrans, &camScale);
        GetTransformMtx(camMatrix, NegVec2(camPos), 0, ToVec2(camZoom, camZoom));

        // --------------------
        // World Border
        // --------------------
        AEMtx33 borderScale, borderMatrix;
        AEMtx33Scale(&borderScale, mapWidth, mapHeight);
        AEMtx33Concat(&borderMatrix, &camMatrix, &borderScale);

        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        AEGfxSetTransform(borderMatrix.m);
        AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

        // --------------------
        // Player Circle (world)
        // --------------------
        AEMtx33 playerScale, playerTrans, playerMatrix;
        AEMtx33Scale(&playerScale, playerRadius, playerRadius);
        AEMtx33Trans(&playerTrans, playerPos.x, playerPos.y);
        AEMtx33Concat(&playerMatrix, &camMatrix, &playerScale);
        AEMtx33Concat(&playerMatrix, &playerTrans, &playerMatrix);

        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        AEGfxSetTransform(playerMatrix.m);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        // --------------------
        // Minimap setup
        // --------------------
        float winW = (float)AEGfxGetWinMaxX();
        float winH = (float)AEGfxGetWinMaxY();
        float scaleX = minimapWidth / mapWidth;
        float scaleY = minimapHeight / mapHeight;
        float mmX = winW - minimapWidth * 0.5f - minimapMargin;
        float mmY = winH - minimapHeight * 0.5f - minimapMargin;

        // --------------------
        // Minimap Border
        // --------------------
        AEMtx33 mmBorderScale, mmBorderTrans, mmBorderMatrix;
        AEMtx33Scale(&mmBorderScale, mapWidth * scaleX, mapHeight * scaleY);
        AEMtx33Trans(&mmBorderTrans, mmX, mmY);
        AEMtx33Concat(&mmBorderMatrix, &mmBorderTrans, &mmBorderScale);

        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        AEGfxSetTransform(mmBorderMatrix.m);
        AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

        // --------------------
        // Minimap Player Dot 
        // --------------------
        AEMtx33 mmPlayerScale, mmPlayerTrans, mmPlayerMatrix;
        float dotScaleX = (playerRadius * scaleX) * minimapPlayerScaleFactor;
        float dotScaleY = (playerRadius * scaleY) * minimapPlayerScaleFactor;
        AEMtx33Scale(&mmPlayerScale, dotScaleX, dotScaleY);
        AEMtx33Trans(&mmPlayerTrans, mmX + playerPos.x * scaleX, mmY + playerPos.y * scaleY);
        AEMtx33Concat(&mmPlayerMatrix, &mmPlayerTrans, &mmPlayerScale);

        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        AEGfxSetTransform(mmPlayerMatrix.m);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        // --------------------
        // Minimap Arrow 
        // --------------------
        DrawMinimapArrow(mmX, mmY, scaleX, scaleY);
    }

    //TESTING
    GameObject* go, * enemy;
}

void GameState::LoadState()
{
    bgm.Init();
    bgm.PlayNormal();

    halfMapWidth = mapWidth * 0.5f;
    halfMapHeight = mapHeight * 0.5f;

    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);
    borderMesh = CreateBorderRectMesh(); 

    AEVec2Zero(&playerPos);
    playerDir = { 1.0f, 0.0f };

    AEVec2Zero(&camPos);
    AEVec2Zero(&camVel);

    //Testing
    GameObjectManager::GetInstance()->InitCollisionGrid(mapWidth, mapHeight);
    go = new GameObject;
    go->Init({200,200}, { 100,100 }, 0, MESH_SQUARE_ANIM, COL_RECT, { 100,100 }, CreateBitmask(1, GameObject::ENEMIES), GameObject::COLLISION_LAYER::PLAYER);
    go->GetRenderData().InitAnimation(6, 9)
        ->LoopAnim()
        ->AddTexture("Assets/sprite_test.png");

    enemy = new GameObject;
    enemy->Init({ 200,0 }, { 100,-100 }, 0, MESH_CIRCLE, COL_CIRCLE, { 100,100 }, CreateBitmask(1, GameObject::PLAYER), GameObject::ENEMIES);
}

void GameState::InitState()
{
	std::cout << "Game state enter\n";
}

void GameState::ExitState()
{
}

void GameState::UnloadState()
{
    if (circleMesh) { /*AEGfxMeshFree(circleMesh);*/ circleMesh = nullptr; }
    if (borderMesh) { AEGfxMeshFree(borderMesh); borderMesh = nullptr; }
    bgm.Exit();
}

void GameState::Update(double dt)
{
    // --- Player movement (WASD) ---
    AEVec2 movement = { 0.0f, 0.0f };
    if (AEInputCheckCurr(AEVK_W)) movement.y += 5.0f;
    if (AEInputCheckCurr(AEVK_S)) movement.y -= 5.0f;
    if (AEInputCheckCurr(AEVK_A)) movement.x -= 5.0f;
    if (AEInputCheckCurr(AEVK_D)) movement.x += 5.0f;

    f32 len = AEVec2Length(&movement);
    if (len > 0.0f) {
        // Normalize movement to get direction
        AEVec2 dirN = movement;
        AEVec2Scale(&dirN, &dirN, 1.0f / len);
        playerDir = dirN;

        // Scale by speed and delta time
        AEVec2Scale(&movement, &movement, playerSpeed * static_cast<float>(dt) / len);
        AEVec2Add(&playerPos, &playerPos, &movement);
    }

    // Clamp player inside map bounds
    playerPos.x = AEClamp(playerPos.x, -halfMapWidth + playerRadius, halfMapWidth - playerRadius);
    playerPos.y = AEClamp(playerPos.y, -halfMapHeight + playerRadius, halfMapHeight - playerRadius);

    UpdateWorldMap(static_cast<float>(dt));

    GameObjectManager::GetInstance()->UpdateObjects(dt);
}

void GameState::Draw()
{
    RenderWorldMap();
    GameObjectManager::GetInstance()->DrawObjects();
}
