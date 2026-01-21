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
#include <cmath>

namespace {
    BGMManager bgm;
    AEGfxVertexList* circleMesh = nullptr;
    AEGfxVertexList* borderMesh = nullptr;
    AEGfxVertexList* fogTileMesh = nullptr;

    AEVec2 playerPos;
    AEVec2 playerDir = { 1.0f, 0.0f };
    float playerRadius = 15.0f;
    float playerSpeed = 300.0f;

    // Camera State
    AEVec2 camPos, camVel;
    float camZoom = 2.0f;
    float camSmoothTime = 0.15f;

    // Map Dimensions
    float mapWidth = 2000.0f;
    float mapHeight = 2000.0f;
    float halfMapWidth, halfMapHeight;

    // --- Fog of War System ---
    const int FOG_GRID_SIZE = 40;
    float discoveryGrid[FOG_GRID_SIZE][FOG_GRID_SIZE];  // 1.0 = clear, 0.0 = hidden
    float regenTimerGrid[FOG_GRID_SIZE][FOG_GRID_SIZE]; // Seconds remaining before regen

    const float REGEN_DELAY = 3.0f;    // 3 seconds stay clear
    const float FOG_REGEN_RATE = 0.2f; // Fade out speed
    float tileWorldSizeX, tileWorldSizeY;

    // Minimap Settings
    float minimapWidth = 200.0f;
    float minimapHeight = 200.0f;
    float minimapMargin = 20.0f;
    float minimapPlayerScaleFactor = 4.0f;
    unsigned int minimapArrowColor = 0xFF00FFFF;

    // Helper for Fog Logic
    void UpdateDiscovery(float dt) {
        // 1. Handle Regeneration logic for all tiles
        for (int x = 0; x < FOG_GRID_SIZE; ++x) {
            for (int y = 0; y < FOG_GRID_SIZE; ++y) {
                if (regenTimerGrid[x][y] > 0.0f) {
                    regenTimerGrid[x][y] -= dt;
                }
                else {
                    discoveryGrid[x][y] -= FOG_REGEN_RATE * dt;
                    if (discoveryGrid[x][y] < 0.0f) discoveryGrid[x][y] = 0.0f;
                }
            }
        }

        // 2. Player clears fog in a radius
        int gridX = (int)((playerPos.x + halfMapWidth) / tileWorldSizeX);
        int gridY = (int)((playerPos.y + halfMapHeight) / tileWorldSizeY);
        const int RADIUS = 3;

        for (int x = gridX - RADIUS; x <= gridX + RADIUS; ++x) {
            for (int y = gridY - RADIUS; y <= gridY + RADIUS; ++y) {
                if (x >= 0 && x < FOG_GRID_SIZE && y >= 0 && y < FOG_GRID_SIZE) {
                    discoveryGrid[x][y] = 1.0f;        // Reset visibility to max
                    regenTimerGrid[x][y] = REGEN_DELAY; // Reset 3s timer
                }
            }
        }
    }

    void DrawMinimapArrow(float mmX, float mmY, float scaleX, float scaleY) {
        float cx = mmX + playerPos.x * scaleX;
        float cy = mmY + playerPos.y * scaleY;

        float dx = playerDir.x, dy = playerDir.y;
        float mag = sqrtf(dx * dx + dy * dy);
        if (mag < 1e-4f) { dx = 1.0f; dy = 0.0f; }
        else { dx /= mag; dy /= mag; }

        float px = -dy, py = dx;
        float dotRadius = playerRadius * 0.5f * (scaleX + scaleY) * minimapPlayerScaleFactor;
        float offset = dotRadius + 5.0f;
        float ax = cx + dx * offset, ay = cy + dy * offset;

        float tipX = ax + dx * 10.0f, tipY = ay + dy * 10.0f;
        float bLX = ax - dx * 2.0f + px * 4.0f, bLY = ay - dy * 2.0f + py * 4.0f;
        float bRX = ax - dx * 2.0f - px * 4.0f, bRY = ay - dy * 2.0f - py * 4.0f;

        AEGfxMeshStart();
        AEGfxTriAdd(tipX, tipY, minimapArrowColor, 0, 0, bLX, bLY, minimapArrowColor, 0, 0, bRX, bRY, minimapArrowColor, 0, 0);
        AEGfxVertexList* arrowMesh = AEGfxMeshEnd();

        AEMtx33 identity; AEMtx33Identity(&identity);
        AEGfxSetTransform(identity.m);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxMeshDraw(arrowMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(arrowMesh);
    }

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
        float winW = (float)AEGfxGetWinMaxX();
        float winH = (float)AEGfxGetWinMaxY();
        float viewHalfW = (winW * 0.5f) / camZoom;
        float viewHalfH = (winH * 0.5f) / camZoom;

        AEVec2 camTarget = playerPos;
        float limitX = halfMapWidth - viewHalfW;
        float limitY = halfMapHeight - viewHalfH;

        if (limitX > 0) camTarget.x = AEClamp(camTarget.x, -limitX, limitX);
        else camTarget.x = 0;
        if (limitY > 0) camTarget.y = AEClamp(camTarget.y, -limitY, limitY);
        else camTarget.y = 0;

        float omega = 2.0f / camSmoothTime;
        float xd = omega * dt;
        float expK = 1.0f / (1.0f + xd + 0.48f * xd * xd + 0.235f * xd * xd * xd);
        AEVec2 change = { camPos.x - camTarget.x, camPos.y - camTarget.y };
        AEVec2 temp = { (camVel.x + omega * change.x) * dt, (camVel.y + omega * change.y) * dt };

        camVel.x = (camVel.x - omega * temp.x) * expK;
        camVel.y = (camVel.y - omega * temp.y) * expK;
        camPos.x = camTarget.x + (change.x + temp.x) * expK;
        camPos.y = camTarget.y + (change.y + temp.y) * expK;

        SetCameraPos(camPos);
        SetCamZoom(camZoom);
    }

    void RenderWorldMap(void) {
        AEGfxStart();
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        AEMtx33 viewMtx, zoomMtx, camMatrix;
        AEMtx33Trans(&viewMtx, -camPos.x, -camPos.y);
        AEMtx33Scale(&zoomMtx, camZoom, camZoom);
        AEMtx33Concat(&camMatrix, &zoomMtx, &viewMtx);

        // 1. World Border
        AEMtx33 borderScale, borderMatrix;
        AEMtx33Scale(&borderScale, mapWidth, mapHeight);
        AEMtx33Concat(&borderMatrix, &camMatrix, &borderScale);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransform(borderMatrix.m);
        AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

        // 2. Player
        AEMtx33 pScale, pTrans, pResult;
        AEMtx33Scale(&pScale, playerRadius * 2.0f, playerRadius * 2.0f);
        AEMtx33Trans(&pTrans, playerPos.x, playerPos.y);
        AEMtx33Concat(&pResult, &camMatrix, &pTrans);
        AEMtx33Concat(&pResult, &pResult, &pScale);
        AEGfxSetTransform(pResult.m);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        // --- UI SECTION (Minimap) ---
        float winW = (float)AEGfxGetWinMaxX(), winH = (float)AEGfxGetWinMaxY();
        float scaleX = minimapWidth / mapWidth, scaleY = minimapHeight / mapHeight;
        float mmX = winW - minimapWidth * 0.5f - minimapMargin;
        float mmY = winH - minimapHeight * 0.5f - minimapMargin;

        for (int x = 0; x < FOG_GRID_SIZE; ++x) {
            for (int y = 0; y < FOG_GRID_SIZE; ++y) {
                if (discoveryGrid[x][y] < 1.0f) {
                    float fogOpacity = 1.0f - discoveryGrid[x][y];
                    float oX = (x * tileWorldSizeX) - halfMapWidth + (tileWorldSizeX * 0.5f);
                    float oY = (y * tileWorldSizeY) - halfMapHeight + (tileWorldSizeY * 0.5f);

                    AEMtx33 s, t, final;
                    AEMtx33Scale(&s, tileWorldSizeX * scaleX, tileWorldSizeY * scaleY);
                    AEMtx33Trans(&t, mmX + oX * scaleX, mmY + oY * scaleY);
                    AEMtx33Concat(&final, &t, &s);

                    AEGfxSetTransform(final.m);
                    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                    // Tint white mesh by opacity using your required function
                    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, fogOpacity);
                    AEGfxMeshDraw(fogTileMesh, AE_GFX_MDM_TRIANGLES);
                }
            }
        }

        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEMtx33 mmPScale, mmPTrans, mmPMatrix;
        AEMtx33Scale(&mmPScale, (playerRadius * scaleX) * minimapPlayerScaleFactor, (playerRadius * scaleY) * minimapPlayerScaleFactor);
        AEMtx33Trans(&mmPTrans, mmX + playerPos.x * scaleX, mmY + playerPos.y * scaleY);
        AEMtx33Concat(&mmPMatrix, &mmPTrans, &mmPScale);
        AEGfxSetTransform(mmPMatrix.m);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        DrawMinimapArrow(mmX, mmY, scaleX, scaleY);

        AEMtx33 mmBScale, mmBTrans, mmBMatrix;
        AEMtx33Scale(&mmBScale, minimapWidth, minimapHeight);
        AEMtx33Trans(&mmBTrans, mmX, mmY);
        AEMtx33Concat(&mmBMatrix, &mmBTrans, &mmBScale);
        AEGfxSetTransform(mmBMatrix.m);
        AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);
    }
}

void GameState::LoadState() {
    bgm.Init(); bgm.PlayNormal();
    halfMapWidth = mapWidth * 0.5f; halfMapHeight = mapHeight * 0.5f;
    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);
    borderMesh = CreateBorderRectMesh();
    tileWorldSizeX = mapWidth / (float)FOG_GRID_SIZE;
    tileWorldSizeY = mapHeight / (float)FOG_GRID_SIZE;

    AEGfxMeshStart();
    // Vertex color at 0xFFFFFFFF allows SetColorToMultiply to work best
    AEGfxTriAdd(-0.5f, -0.5f, 0x88666666, 0, 0, 0.5f, -0.5f, 0x88666666, 0, 0, 0.5f, 0.5f, 0x88666666, 0, 0);
    AEGfxTriAdd(-0.5f, -0.5f, 0x88666666, 0, 0, 0.5f, 0.5f, 0x88666666, 0, 0, -0.5f, 0.5f, 0x88666666, 0, 0);
    fogTileMesh = AEGfxMeshEnd();
}

void GameState::InitState() {
    playerPos = { 0,0 };
    camPos = playerPos; camVel = { 0,0 };
    for (int i = 0; i < FOG_GRID_SIZE; ++i) {
        for (int j = 0; j < FOG_GRID_SIZE; ++j) {
            discoveryGrid[i][j] = 0.0f;
            regenTimerGrid[i][j] = 0.0f;
        }
    }
    UpdateDiscovery(0.016f);
}

void GameState::Update(double dt) {
    AEVec2 move = { 0,0 };
    if (AEInputCheckCurr(AEVK_W)) move.y += 1;
    if (AEInputCheckCurr(AEVK_S)) move.y -= 1;
    if (AEInputCheckCurr(AEVK_A)) move.x -= 1;
    if (AEInputCheckCurr(AEVK_D)) move.x += 1;

    if (AEVec2Length(&move) > 0) {
        AEVec2Normalize(&playerDir, &move);
        AEVec2Scale(&move, &playerDir, playerSpeed * (float)dt);
        AEVec2Add(&playerPos, &playerPos, &move);
    }

    playerPos.x = AEClamp(playerPos.x, -halfMapWidth + playerRadius, halfMapWidth - playerRadius);
    playerPos.y = AEClamp(playerPos.y, -halfMapHeight + playerRadius, halfMapHeight - playerRadius);

    UpdateDiscovery((float)dt);
    UpdateWorldMap((float)dt);
}

void GameState::Draw() { RenderWorldMap(); }

void GameState::ExitState() {}

void GameState::UnloadState() {
    if (borderMesh) AEGfxMeshFree(borderMesh);
    if (fogTileMesh) AEGfxMeshFree(fogTileMesh);
    bgm.Exit();
}