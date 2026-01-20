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
    // --- Audio ---
    BGMManager bgm;

    // --- Rendering Assets ---
    AEGfxVertexList* circleMesh = nullptr;
    AEGfxVertexList* borderMesh = nullptr;
    AEGfxVertexList* fogTileMesh = nullptr;

    // --- Player State ---
    AEVec2 playerPos;
    AEVec2 playerDir = { 1.0f, 0.0f };
    float playerRadius = 15.0f;
    float playerSpeed = 300.0f;

    // --- Spawn Point Config ---
    // {1.0, -1.0} = Bottom Right corner
    AEVec2 spawnMultiplier = { 1.0f, -1.0f };

    // --- Camera State ---
    AEVec2 camPos;
    AEVec2 camVel;
    float camZoom = 1.0f;
    float camSmoothTime = 0.25f;
    float softMargin = 100.0f;

    // --- Map Dimensions ---
    float mapWidth = 2000.0f;
    float mapHeight = 2000.0f;
    float halfMapWidth;
    float halfMapHeight;

    // --- Fog of War Settings ---
    const int FOG_GRID_SIZE = 40;
    bool discoveryGrid[FOG_GRID_SIZE][FOG_GRID_SIZE];
    float tileWorldSizeX;
    float tileWorldSizeY;

    // --- Minimap Settings ---
    float minimapWidth = 200.0f;
    float minimapHeight = 200.0f;
    float minimapMargin = 20.0f;
    float minimapPlayerScaleFactor = 4.0f;

    // --- Minimap Arrow Settings ---
    float minimapArrowLengthPx = 10.0f;
    float minimapArrowThicknessPx = 8.0f;
    float minimapArrowExtraOffsetPx = 8.0f;
    unsigned int minimapArrowColor = 0xFF00FFFF;

    // --- Helper Math ---
    inline float Saturate(float x) { return AEClamp(x, 0.0f, 1.0f); }
    inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

    // Reveal 7x7 area around player
    void UpdateDiscovery() {
        int gridX = (int)((playerPos.x + halfMapWidth) / tileWorldSizeX);
        int gridY = (int)((playerPos.y + halfMapHeight) / tileWorldSizeY);

        const int RADIUS = 3;
        for (int x = gridX - RADIUS; x <= gridX + RADIUS; ++x) {
            for (int y = gridY - RADIUS; y <= gridY + RADIUS; ++y) {
                if (x >= 0 && x < FOG_GRID_SIZE && y >= 0 && y < FOG_GRID_SIZE) {
                    discoveryGrid[x][y] = true;
                }
            }
        }
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
        float halfViewW = (winW * 0.5f) / camZoom;
        float halfViewH = (winH * 0.5f) / camZoom;

        float minX = -halfMapWidth + halfViewW;
        float maxX = halfMapWidth - halfViewW;
        float minY = -halfMapHeight + halfViewH;
        float maxY = halfMapHeight - halfViewH;

        if (minX > maxX) minX = maxX = 0.0f;
        if (minY > maxY) minY = maxY = 0.0f;

        AEVec2 camTarget = playerPos;
        float tLeft = 1.0f - Saturate((playerPos.x - (minX + softMargin)) / softMargin);
        float tRight = 1.0f - Saturate(((maxX - softMargin) - playerPos.x) / softMargin);
        float tBottom = 1.0f - Saturate((playerPos.y - (minY + softMargin)) / softMargin);
        float tTop = 1.0f - Saturate(((maxY - softMargin) - playerPos.y) / softMargin);

        camTarget.x = Lerp(playerPos.x, minX, tLeft);
        camTarget.x = Lerp(camTarget.x, maxX, tRight);
        camTarget.y = Lerp(playerPos.y, minY, tBottom);
        camTarget.y = Lerp(camTarget.y, maxY, tTop);

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

    void DrawMinimapArrow(float mmX, float mmY, float scaleX, float scaleY) {
        float cx = mmX + playerPos.x * scaleX;
        float cy = mmY + playerPos.y * scaleY;
        float dx = playerDir.x, dy = playerDir.y;
        float mag = sqrtf(dx * dx + dy * dy);
        if (mag < 1e-4f) { dx = 1.0f; dy = 0.0f; }
        else { dx /= mag; dy /= mag; }

        float px = -dy, py = dx;
        float dotRadius = playerRadius * 0.5f * (scaleX + scaleY) * minimapPlayerScaleFactor;
        float offset = dotRadius + minimapArrowExtraOffsetPx;
        float ax = cx + dx * offset, ay = cy + dy * offset;

        float tipX = ax + dx * minimapArrowLengthPx;
        float tipY = ay + dy * minimapArrowLengthPx;
        float bLX = ax - dx * 2.0f + px * (minimapArrowThicknessPx * 0.5f);
        float bLY = ay - dy * 2.0f + py * (minimapArrowThicknessPx * 0.5f);
        float bRX = ax - dx * 2.0f - px * (minimapArrowThicknessPx * 0.5f);
        float bRY = ay - dy * 2.0f - py * (minimapArrowThicknessPx * 0.5f);

        AEGfxMeshStart();
        AEGfxTriAdd(tipX, tipY, minimapArrowColor, 0, 0, bLX, bLY, minimapArrowColor, 0, 0, bRX, bRY, minimapArrowColor, 0, 0);
        AEGfxVertexList* arrowMesh = AEGfxMeshEnd();
        AEMtx33 identity; AEMtx33Identity(&identity);
        AEGfxSetTransform(identity.m);
        AEGfxMeshDraw(arrowMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(arrowMesh);
    }

    void RenderWorldMap(void) {
        AEGfxStart();
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        AEMtx33 camMatrix;
        GetTransformMtx(camMatrix, NegVec2(camPos), 0, ToVec2(camZoom, camZoom));

        // World Border
        AEMtx33 borderScale, borderMatrix;
        AEMtx33Scale(&borderScale, mapWidth, mapHeight);
        AEMtx33Concat(&borderMatrix, &camMatrix, &borderScale);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransform(borderMatrix.m);
        AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

        // Main Game Player
        AEMtx33 worldPScale, worldPTrans, worldPMatrix;
        AEMtx33Scale(&worldPScale, playerRadius * 2.0f, playerRadius * 2.0f);
        AEMtx33Trans(&worldPTrans, playerPos.x, playerPos.y);
        AEMtx33Concat(&worldPMatrix, &camMatrix, &worldPTrans);
        AEMtx33Concat(&worldPMatrix, &worldPMatrix, &worldPScale);
        AEGfxSetTransform(worldPMatrix.m);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        // --- Minimap UI ---
        float winW = (float)AEGfxGetWinMaxX(), winH = (float)AEGfxGetWinMaxY();
        float scaleX = minimapWidth / mapWidth, scaleY = minimapHeight / mapHeight;
        float mmX = winW - minimapWidth * 0.5f - minimapMargin;
        float mmY = winH - minimapHeight * 0.5f - minimapMargin;

        // Fog on Minimap
        float mmTileW = tileWorldSizeX * scaleX;
        float mmTileH = tileWorldSizeY * scaleY;
        for (int x = 0; x < FOG_GRID_SIZE; ++x) {
            for (int y = 0; y < FOG_GRID_SIZE; ++y) {
                if (!discoveryGrid[x][y]) {
                    float oX = (x * tileWorldSizeX) - halfMapWidth + (tileWorldSizeX * 0.5f);
                    float oY = (y * tileWorldSizeY) - halfMapHeight + (tileWorldSizeY * 0.5f);
                    AEMtx33 s, t, final;
                    AEMtx33Scale(&s, mmTileW, mmTileH);
                    AEMtx33Trans(&t, mmX + oX * scaleX, mmY + oY * scaleY);
                    AEMtx33Concat(&final, &t, &s);
                    AEGfxSetTransform(final.m);
                    AEGfxMeshDraw(fogTileMesh, AE_GFX_MDM_TRIANGLES);
                }
            }
        }

        // Minimap Player
        AEMtx33 mmPScale, mmPTrans, mmPMatrix;
        AEMtx33Scale(&mmPScale, (playerRadius * scaleX) * minimapPlayerScaleFactor, (playerRadius * scaleY) * minimapPlayerScaleFactor);
        AEMtx33Trans(&mmPTrans, mmX + playerPos.x * scaleX, mmY + playerPos.y * scaleY);
        AEMtx33Concat(&mmPMatrix, &mmPTrans, &mmPScale);
        AEGfxSetTransform(mmPMatrix.m);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        DrawMinimapArrow(mmX, mmY, scaleX, scaleY);

        // Minimap Border
        AEMtx33 mmBScale, mmBTrans, mmBMatrix;
        AEMtx33Scale(&mmBScale, minimapWidth, minimapHeight);
        AEMtx33Trans(&mmBTrans, mmX, mmY);
        AEMtx33Concat(&mmBMatrix, &mmBTrans, &mmBScale);
        AEGfxSetTransform(mmBMatrix.m);
        AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);
    }
}

// --- Implementation ---

void GameState::LoadState() {
    bgm.Init();
    bgm.PlayNormal();
    halfMapWidth = mapWidth * 0.5f;
    halfMapHeight = mapHeight * 0.5f;

    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);
    borderMesh = CreateBorderRectMesh();

    tileWorldSizeX = mapWidth / (float)FOG_GRID_SIZE;
    tileWorldSizeY = mapHeight / (float)FOG_GRID_SIZE;

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0x88666666, 0, 0, 0.5f, -0.5f, 0x88666666, 0, 0, 0.5f, 0.5f, 0x88666666, 0, 0);
    AEGfxTriAdd(-0.5f, -0.5f, 0x88666666, 0, 0, 0.5f, 0.5f, 0x88666666, 0, 0, -0.5f, 0.5f, 0x88666666, 0, 0);
    fogTileMesh = AEGfxMeshEnd();
}

void GameState::InitState() {
    // SPAWN BOTTOM RIGHT
    playerPos.x = (halfMapWidth - playerRadius) * spawnMultiplier.x;
    playerPos.y = (halfMapHeight - playerRadius) * spawnMultiplier.y;

    camPos = playerPos;
    camVel = { 0, 0 };

    for (int i = 0; i < FOG_GRID_SIZE; ++i)
        for (int j = 0; j < FOG_GRID_SIZE; ++j) discoveryGrid[i][j] = false;

    UpdateDiscovery();
}

void GameState::Update(double dt) {
    AEVec2 movement = { 0.0f, 0.0f };
    if (AEInputCheckCurr(AEVK_W)) movement.y += 1.0f;
    if (AEInputCheckCurr(AEVK_S)) movement.y -= 1.0f;
    if (AEInputCheckCurr(AEVK_A)) movement.x -= 1.0f;
    if (AEInputCheckCurr(AEVK_D)) movement.x += 1.0f;

    float len = AEVec2Length(&movement);
    if (len > 0.0f) {
        AEVec2Normalize(&playerDir, &movement);
        AEVec2Scale(&movement, &playerDir, playerSpeed * (float)dt);
        AEVec2Add(&playerPos, &playerPos, &movement);
    }

    playerPos.x = AEClamp(playerPos.x, -halfMapWidth + playerRadius, halfMapWidth - playerRadius);
    playerPos.y = AEClamp(playerPos.y, -halfMapHeight + playerRadius, halfMapHeight - playerRadius);

    // --- DEBUG KEYS ---
    if (AEInputCheckTriggered(AEVK_F)) { // Clear Fog
        for (int i = 0; i < FOG_GRID_SIZE; ++i)
            for (int j = 0; j < FOG_GRID_SIZE; ++j) discoveryGrid[i][j] = true;
    }
    if (AEInputCheckTriggered(AEVK_R)) { // Respawn
        InitState();
    }

    UpdateDiscovery();
    UpdateWorldMap((float)dt);
}

void GameState::Draw() {
    RenderWorldMap();
}

void GameState::ExitState() {}

void GameState::UnloadState() {
    if (borderMesh) AEGfxMeshFree(borderMesh);
    if (fogTileMesh) AEGfxMeshFree(fogTileMesh);
    bgm.Exit();
}