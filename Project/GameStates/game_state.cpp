#include "game_state.h"
#include "../Music.h"
#include "../helpers/render_utils.h"
#include "../helpers/matrix_utils.h"
#include "../helpers/vec2_utils.h"
#include "../helpers/coord_utils.h"
#include "../camera.h"
#include "../rendering_manager.h"
#include "../GameObjects/GameObject.h"
#include "../helpers/collision.h"
#include "../Map.h"
#include <iostream>
#include <cmath>

namespace {
    BGMManager bgm;
    AEGfxVertexList* circleMesh = nullptr;
    AEGfxVertexList* borderMesh = nullptr;
    AEGfxVertexList* fogTileMesh = nullptr;
    AEGfxVertexList* wallMesh = nullptr;

    AEVec2 playerPos;
    AEVec2 playerDir = { 1.0f, 0.0f };
    float playerRadius = 15.0f;
    float playerSpeed = 300.0f;

    bool bossAlive = true;
    float bossRadius = 60.0f;

    AEVec2 camPos, camVel;
    float camZoom = 1.2f;
    float camSmoothTime = 0.15f;

    float mapWidth = 2400.0f;
    float mapHeight = 2000.0f;
    float halfMapWidth, halfMapHeight;
    MapData currentLevel;

    const int FOG_GRID_SIZE = 40;
    float discoveryGrid[FOG_GRID_SIZE][FOG_GRID_SIZE];
    float regenTimerGrid[FOG_GRID_SIZE][FOG_GRID_SIZE];
    const float REGEN_DELAY = 5.0f;
    const float FOG_REGEN_RATE = 0.3f;
    float tileWorldSizeX, tileWorldSizeY;

    float minimapWidth = 200.0f;
    float minimapHeight = 200.0f;
    float minimapMargin = 20.0f;
    float minimapPlayerScaleFactor = 4.0f;

    void UpdateDiscovery(float dt) {
        float revealRadiusWorld = 180.0f;
        for (int x = 0; x < FOG_GRID_SIZE; ++x) {
            for (int y = 0; y < FOG_GRID_SIZE; ++y) {
                if (regenTimerGrid[x][y] > 0.0f) regenTimerGrid[x][y] -= dt;
                else {
                    discoveryGrid[x][y] -= FOG_REGEN_RATE * dt;
                    if (discoveryGrid[x][y] < 0.0f) discoveryGrid[x][y] = 0.0f;
                }
                float tileWorldX = (x * tileWorldSizeX) - halfMapWidth + (tileWorldSizeX * 0.5f);
                float tileWorldY = (y * tileWorldSizeY) - halfMapHeight + (tileWorldSizeY * 0.5f);
                float dx = playerPos.x - tileWorldX;
                float dy = playerPos.y - tileWorldY;
                if ((dx * dx + dy * dy) < (revealRadiusWorld * revealRadiusWorld)) {
                    discoveryGrid[x][y] = 1.0f;
                    regenTimerGrid[x][y] = REGEN_DELAY;
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
        AEGfxTriAdd(tipX, tipY, 0xFF00FFFF, 0, 0, bLX, bLY, 0xFF00FFFF, 0, 0, bRX, bRY, 0xFF00FFFF, 0, 0);
        AEGfxVertexList* arrowMesh = AEGfxMeshEnd();

        AEMtx33 identity; AEMtx33Identity(&identity);
        AEGfxSetTransform(identity.m);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxMeshDraw(arrowMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(arrowMesh);
    }

    void UpdateWorldMap(float dt) {
        float winW = (float)AEGfxGetWinMaxX(), winH = (float)AEGfxGetWinMaxY();
        float viewHalfW = (winW * 0.5f) / camZoom, viewHalfH = (winH * 0.5f) / camZoom;
        AEVec2 camTarget = playerPos;
        float limitX = halfMapWidth - viewHalfW, limitY = halfMapHeight - viewHalfH;
        if (limitX > 0) camTarget.x = AEClamp(camTarget.x, -limitX, limitX); else camTarget.x = 0;
        if (limitY > 0) camTarget.y = AEClamp(camTarget.y, -limitY, limitY); else camTarget.y = 0;

        float omega = 2.0f / camSmoothTime, xd = omega * dt;
        float expK = 1.0f / (1.0f + xd + 0.48f * xd * xd + 0.235f * xd * xd * xd);
        AEVec2 change = { camPos.x - camTarget.x, camPos.y - camTarget.y };
        AEVec2 temp = { (camVel.x + omega * change.x) * dt, (camVel.y + omega * change.y) * dt };
        camVel.x = (camVel.x - omega * temp.x) * expK; camVel.y = (camVel.y - omega * temp.y) * expK;
        camPos.x = camTarget.x + (change.x + temp.x) * expK; camPos.y = camTarget.y + (change.y + temp.y) * expK;
        SetCameraPos(camPos); SetCamZoom(camZoom);
    }

    void RenderWorldMap() {
        AEGfxStart();
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        AEMtx33 viewMtx, zoomMtx, camMatrix;
        AEMtx33Trans(&viewMtx, -camPos.x, -camPos.y);
        AEMtx33Scale(&zoomMtx, camZoom, camZoom);
        AEMtx33Concat(&camMatrix, &zoomMtx, &viewMtx);

        // --- WORLD ---
        AEGfxSetColorToMultiply(0.4f, 0.4f, 0.4f, 1.0f);
        for (const auto& wall : currentLevel.walls) {
            AEMtx33 s, t, final;
            AEMtx33Scale(&s, wall.width, wall.height);
            AEMtx33Trans(&t, wall.position.x, wall.position.y);
            AEMtx33Concat(&final, &camMatrix, &t);
            AEMtx33Concat(&final, &final, &s);
            AEGfxSetTransform(final.m);
            AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
        }

        if (bossAlive) {
            AEMtx33 bS, bT, bF; AEMtx33Scale(&bS, bossRadius * 2, bossRadius * 2); AEMtx33Trans(&bT, currentLevel.doorPos.x, currentLevel.doorPos.y);
            AEMtx33Concat(&bF, &camMatrix, &bT); AEMtx33Concat(&bF, &bF, &bS); AEGfxSetTransform(bF.m);
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

            AEMtx33 dS, dT, dF; AEMtx33Scale(&dS, 40.0f, 120.0f); AEMtx33Trans(&dT, currentLevel.doorPos.x - 335.0f, currentLevel.doorPos.y);
            AEMtx33Concat(&dF, &camMatrix, &dT); AEMtx33Concat(&dF, &dF, &dS); AEGfxSetTransform(dF.m);
            AEGfxSetColorToMultiply(0.0f, 0.8f, 0.0f, 1.0f); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
        }

        AEMtx33 cS, cT, cF; AEMtx33Scale(&cS, 35, 35); AEMtx33Trans(&cT, currentLevel.chestPos.x, currentLevel.chestPos.y);
        AEMtx33Concat(&cF, &camMatrix, &cT); AEMtx33Concat(&cF, &cF, &cS); AEGfxSetTransform(cF.m);
        AEGfxSetColorToMultiply(1.0f, 0.84f, 0.0f, 1.0f); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);

        AEVec2 enemies[] = { currentLevel.enemy1Pos, currentLevel.enemy2Pos };
        for (auto& e : enemies) {
            AEMtx33 eS, eT, eF; AEMtx33Scale(&eS, 30, 30); AEMtx33Trans(&eT, e.x, e.y);
            AEMtx33Concat(&eF, &camMatrix, &eT); AEMtx33Concat(&eF, &eF, &eS); AEGfxSetTransform(eF.m);
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
        }

        AEMtx33 pS, pT, pFinal; AEMtx33Scale(&pS, playerRadius * 2, playerRadius * 2); AEMtx33Trans(&pT, playerPos.x, playerPos.y);
        AEMtx33Concat(&pFinal, &camMatrix, &pT); AEMtx33Concat(&pFinal, &pFinal, &pS); AEGfxSetTransform(pFinal.m);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        // --- MINIMAP ---
        float scaleX = minimapWidth / mapWidth, scaleY = minimapHeight / mapHeight;
        float mmX = (float)AEGfxGetWinMaxX() - minimapWidth * 0.5f - minimapMargin;
        float mmY = (float)AEGfxGetWinMaxY() - minimapHeight * 0.5f - minimapMargin;

        // 1. Walls on Minimap
        AEGfxSetColorToMultiply(0.4f, 0.4f, 0.4f, 1.0f);
        for (const auto& w : currentLevel.walls) {
            AEMtx33 s, t, f; AEMtx33Scale(&s, w.width * scaleX, w.height * scaleY);
            AEMtx33Trans(&t, mmX + w.position.x * scaleX, mmY + w.position.y * scaleY);
            AEMtx33Concat(&f, &t, &s); AEGfxSetTransform(f.m); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
        }

        // 2. Boss & Door on Minimap (FIXED)
        if (bossAlive) {
            AEMtx33 bms, bmt, bmf;
            AEMtx33Scale(&bms, (bossRadius * 2) * scaleX, (bossRadius * 2) * scaleY);
            AEMtx33Trans(&bmt, mmX + currentLevel.doorPos.x * scaleX, mmY + currentLevel.doorPos.y * scaleY);
            AEMtx33Concat(&bmf, &bmt, &bms); AEGfxSetTransform(bmf.m);
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

            AEMtx33 dms, dmt, dmf;
            AEMtx33Scale(&dms, 40.0f * scaleX, 120.0f * scaleY);
            AEMtx33Trans(&dmt, mmX + (currentLevel.doorPos.x - 335.0f) * scaleX, mmY + currentLevel.doorPos.y * scaleY);
            AEMtx33Concat(&dmf, &dmt, &dms); AEGfxSetTransform(dmf.m);
            AEGfxSetColorToMultiply(0.0f, 0.8f, 0.0f, 1.0f); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
        }

        // 3. Chest & Enemies on Minimap
        AEMtx33 cs, ct, cf; AEMtx33Scale(&cs, 35 * scaleX, 35 * scaleY);
        AEMtx33Trans(&ct, mmX + currentLevel.chestPos.x * scaleX, mmY + currentLevel.chestPos.y * scaleY);
        AEMtx33Concat(&cf, &ct, &cs); AEGfxSetTransform(cf.m);
        AEGfxSetColorToMultiply(1.0f, 0.84f, 0.0f, 1.0f); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);

        for (auto& e : enemies) {
            AEMtx33 es, et, ef; AEMtx33Scale(&es, 30 * scaleX, 30 * scaleY);
            AEMtx33Trans(&et, mmX + e.x * scaleX, mmY + e.y * scaleY);
            AEMtx33Concat(&ef, &et, &es); AEGfxSetTransform(ef.m);
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
        }

        // 4. Fog
        for (int x = 0; x < FOG_GRID_SIZE; x++) {
            for (int y = 0; y < FOG_GRID_SIZE; y++) {
                if (discoveryGrid[x][y] < 1.0f) {
                    AEMtx33 s, t, f; AEMtx33Scale(&s, tileWorldSizeX * scaleX, tileWorldSizeY * scaleY);
                    float oX = (x * tileWorldSizeX) - halfMapWidth + tileWorldSizeX * 0.5f;
                    float oY = (y * tileWorldSizeY) - halfMapHeight + tileWorldSizeY * 0.5f;
                    AEMtx33Trans(&t, mmX + oX * scaleX, mmY + oY * scaleY);
                    AEMtx33Concat(&f, &t, &s); AEGfxSetTransform(f.m);
                    AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 1.0f - discoveryGrid[x][y]);
                    AEGfxMeshDraw(fogTileMesh, AE_GFX_MDM_TRIANGLES);
                }
            }
        }

        // 5. Player Dot & Border
        AEMtx33 ps, pt, pf; AEMtx33Scale(&ps, (playerRadius * scaleX) * minimapPlayerScaleFactor, (playerRadius * scaleY) * minimapPlayerScaleFactor);
        AEMtx33Trans(&pt, mmX + playerPos.x * scaleX, mmY + playerPos.y * scaleY);
        AEMtx33Concat(&pf, &pt, &ps); AEGfxSetTransform(pf.m);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
        DrawMinimapArrow(mmX, mmY, scaleX, scaleY);

        AEMtx33 bS, bT, bF; AEMtx33Scale(&bS, minimapWidth, minimapHeight); AEMtx33Trans(&bT, mmX, mmY);
        AEMtx33Concat(&bF, &bT, &bS); AEGfxSetTransform(bF.m);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);
    }
}

void GameState::LoadState() {
    bgm.Init(); bgm.PlayNormal();
    halfMapWidth = mapWidth * 0.5f; halfMapHeight = mapHeight * 0.5f;
    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);
    AEGfxMeshStart();
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0); AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 0, 0); AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0); borderMesh = AEGfxMeshEnd();
    tileWorldSizeX = mapWidth / (float)FOG_GRID_SIZE; tileWorldSizeY = mapHeight / (float)FOG_GRID_SIZE;
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, 0.5f, 0xFFFFFFFF, 0, 0, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    wallMesh = AEGfxMeshEnd(); fogTileMesh = wallMesh;
}

void GameState::InitState() {
    InitTutorial(currentLevel); playerPos = currentLevel.startPos;
    camPos = playerPos; camVel = { 0,0 };
    for (int i = 0; i < FOG_GRID_SIZE; i++) for (int j = 0; j < FOG_GRID_SIZE; j++) { discoveryGrid[i][j] = 0; regenTimerGrid[i][j] = 0; }
}

void GameState::Update(double dt) {
    AEVec2 move = { 0,0 };
    if (AEInputCheckCurr(AEVK_W)) move.y += 1; if (AEInputCheckCurr(AEVK_S)) move.y -= 1;
    if (AEInputCheckCurr(AEVK_A)) move.x -= 1; if (AEInputCheckCurr(AEVK_D)) move.x += 1;
    if (AEVec2Length(&move) > 0) {
        AEVec2Normalize(&move, &move); playerDir = move;
        float subStep = (playerSpeed * (float)dt) / 10.0f;
        auto IsClear = [&](AEVec2 p) {
            for (const auto& w : currentLevel.walls) if (CircleRectCollision(w.position, { w.width, w.height }, p, playerRadius)) return false;
            if (bossAlive) {
                if (AEVec2Distance(&p, &currentLevel.doorPos) < (playerRadius + bossRadius)) return false;
                if (CircleRectCollision({ currentLevel.doorPos.x - 335.0f, currentLevel.doorPos.y }, { 40, 120 }, p, playerRadius)) return false;
            }
            return true;
            };
        for (int i = 0; i < 10; i++) {
            AEVec2 nextX = { playerPos.x + move.x * subStep, playerPos.y };
            if (IsClear(nextX)) playerPos.x = nextX.x;
            AEVec2 nextY = { playerPos.x, playerPos.y + move.y * subStep };
            if (IsClear(nextY)) playerPos.y = nextY.y;
        }
    }
    playerPos.x = AEClamp(playerPos.x, -halfMapWidth + playerRadius, halfMapWidth - playerRadius);
    playerPos.y = AEClamp(playerPos.y, -halfMapHeight + playerRadius, halfMapHeight - playerRadius);
    UpdateDiscovery((float)dt); UpdateWorldMap((float)dt);
}

void GameState::Draw() { RenderWorldMap(); }
void GameState::ExitState() {}
void GameState::UnloadState() { if (borderMesh)AEGfxMeshFree(borderMesh); if (wallMesh)AEGfxMeshFree(wallMesh); bgm.Exit(); }