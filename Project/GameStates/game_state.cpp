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
    // --- Systems ---
    BGMManager bgm;
    AEGfxVertexList* circleMesh = nullptr;
    AEGfxVertexList* borderMesh = nullptr;
    AEGfxVertexList* fogTileMesh = nullptr;
    AEGfxVertexList* wallMesh = nullptr;

    // --- Player State ---
    AEVec2 playerPos;
    AEVec2 playerDir = { 1.0f, 0.0f };
    float playerRadius = 15.0f;
    float playerSpeed = 300.0f;

    // --- Camera State ---
    AEVec2 camPos, camVel;
    float camZoom = 1.2f;
    float camSmoothTime = 0.15f;

    // --- Map Settings ---
    float mapWidth = 2000.0f;
    float mapHeight = 2000.0f;
    float halfMapWidth, halfMapHeight;
    MapData currentLevel;

    // --- Fog of War ---
    const int FOG_GRID_SIZE = 40;
    float discoveryGrid[FOG_GRID_SIZE][FOG_GRID_SIZE];
    float regenTimerGrid[FOG_GRID_SIZE][FOG_GRID_SIZE];
    const float REGEN_DELAY = 5.0f;
    const float FOG_REGEN_RATE = 0.3f;
    float tileWorldSizeX, tileWorldSizeY;

    // --- Minimap Settings ---
    float minimapWidth = 200.0f;
    float minimapHeight = 200.0f;
    float minimapMargin = 20.0f;
    float minimapPlayerScaleFactor = 4.0f;

    // --- Fog of War Update ---
    void UpdateDiscovery(float dt) {
        for (int x = 0; x < FOG_GRID_SIZE; ++x) {
            for (int y = 0; y < FOG_GRID_SIZE; ++y) {
                if (regenTimerGrid[x][y] > 0.0f)
                    regenTimerGrid[x][y] -= dt;
                else {
                    discoveryGrid[x][y] -= FOG_REGEN_RATE * dt;
                    if (discoveryGrid[x][y] < 0.0f) discoveryGrid[x][y] = 0.0f;
                }
            }
        }

        int gridX = (int)((playerPos.x + halfMapWidth) / tileWorldSizeX);
        int gridY = (int)((playerPos.y + halfMapHeight) / tileWorldSizeY);
        const int RADIUS = 3;

        for (int x = gridX - RADIUS; x <= gridX + RADIUS; ++x)
            for (int y = gridY - RADIUS; y <= gridY + RADIUS; ++y)
                if (x >= 0 && x < FOG_GRID_SIZE && y >= 0 && y < FOG_GRID_SIZE) {
                    discoveryGrid[x][y] = 1.0f;
                    regenTimerGrid[x][y] = REGEN_DELAY;
                }
    }

    // --- Minimap Arrow ---
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

    // --- Camera Follow ---
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

    // --- Rendering ---
    void RenderWorldMap() {
        AEGfxStart();
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        AEMtx33 viewMtx, zoomMtx, camMatrix;
        AEMtx33Trans(&viewMtx, -camPos.x, -camPos.y);
        AEMtx33Scale(&zoomMtx, camZoom, camZoom);
        AEMtx33Concat(&camMatrix, &zoomMtx, &viewMtx);

        // 1. Border
        AEMtx33 borderScale, borderMatrix;
        AEMtx33Scale(&borderScale, mapWidth, mapHeight);
        AEMtx33Concat(&borderMatrix, &camMatrix, &borderScale);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxSetTransform(borderMatrix.m);
        AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

        // 2. Walls
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

        // 3. Chest
        AEMtx33 cS, cT, cFinal;
        AEMtx33Scale(&cS, 35, 35);
        AEMtx33Trans(&cT, currentLevel.chestPos.x, currentLevel.chestPos.y);
        AEMtx33Concat(&cFinal, &camMatrix, &cT);
        AEMtx33Concat(&cFinal, &cFinal, &cS);
        AEGfxSetTransform(cFinal.m);
        AEGfxSetColorToMultiply(1.0f, 0.84f, 0, 1.0f);
        AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);

        // 4. Enemies
        AEVec2 enemies[] = { currentLevel.enemy1Pos, currentLevel.enemy2Pos };
        for (auto& e : enemies) {
            AEMtx33 eS, eT, eFinal;
            AEMtx33Scale(&eS, 30, 30);
            AEMtx33Trans(&eT, e.x, e.y);
            AEMtx33Concat(&eFinal, &camMatrix, &eT);
            AEMtx33Concat(&eFinal, &eFinal, &eS);
            AEGfxSetTransform(eFinal.m);
            AEGfxSetColorToMultiply(1, 0, 0, 1);
            AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
        }

        // 5. Player
        AEMtx33 pS, pT, pFinal;
        AEMtx33Scale(&pS, playerRadius * 2, playerRadius * 2);
        AEMtx33Trans(&pT, playerPos.x, playerPos.y);
        AEMtx33Concat(&pFinal, &camMatrix, &pT);
        AEMtx33Concat(&pFinal, &pFinal, &pS);
        AEGfxSetTransform(pFinal.m);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        // --- Minimap render (walls, chest, enemies, fog, player, arrow)
        float winW = (float)AEGfxGetWinMaxX(), winH = (float)AEGfxGetWinMaxY();
        float scaleX = minimapWidth / mapWidth;
        float scaleY = minimapHeight / mapHeight;
        float mmX = winW - minimapWidth * 0.5f - minimapMargin;
        float mmY = winH - minimapHeight * 0.5f - minimapMargin;

        // Walls
        AEGfxSetColorToMultiply(0.4f, 0.4f, 0.4f, 1);
        for (const auto& w : currentLevel.walls) {
            AEMtx33 s, t, f;
            AEMtx33Scale(&s, w.width * scaleX, w.height * scaleY);
            AEMtx33Trans(&t, mmX + w.position.x * scaleX, mmY + w.position.y * scaleY);
            AEMtx33Concat(&f, &t, &s);
            AEGfxSetTransform(f.m);
            AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
        }

        // Chest
        AEMtx33 cs, ct, cf;
        AEMtx33Scale(&cs, 8, 8);
        AEMtx33Trans(&ct, mmX + currentLevel.chestPos.x * scaleX, mmY + currentLevel.chestPos.y * scaleY);
        AEMtx33Concat(&cf, &ct, &cs);
        AEGfxSetTransform(cf.m);
        AEGfxSetColorToMultiply(1, 0.84f, 0, 1);
        AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);

        // Enemies
        for (auto& e : enemies) {
            AEMtx33 es, et, ef;
            AEMtx33Scale(&es, 8, 8);
            AEMtx33Trans(&et, mmX + e.x * scaleX, mmY + e.y * scaleY);
            AEMtx33Concat(&ef, &et, &es);
            AEGfxSetTransform(ef.m);
            AEGfxSetColorToMultiply(1, 0, 0, 1);
            AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
        }

        // Fog
        for (int x = 0; x < FOG_GRID_SIZE; x++)
            for (int y = 0; y < FOG_GRID_SIZE; y++)
                if (discoveryGrid[x][y] < 1) {
                    float fogOpacity = 1.0f - discoveryGrid[x][y];
                    float oX = (x * tileWorldSizeX) - halfMapWidth + tileWorldSizeX * 0.5f;
                    float oY = (y * tileWorldSizeY) - halfMapHeight + tileWorldSizeY * 0.5f;
                    AEMtx33 s, t, f;
                    AEMtx33Scale(&s, tileWorldSizeX * scaleX, tileWorldSizeY * scaleY);
                    AEMtx33Trans(&t, mmX + oX * scaleX, mmY + oY * scaleY);
                    AEMtx33Concat(&f, &t, &s);
                    AEGfxSetTransform(f.m);
                    AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, fogOpacity);
                    AEGfxMeshDraw(fogTileMesh, AE_GFX_MDM_TRIANGLES);
                }

        // Player
        AEMtx33 ps, pt, pf;
        AEMtx33Scale(&ps, (playerRadius * scaleX) * minimapPlayerScaleFactor, (playerRadius * scaleY) * minimapPlayerScaleFactor);
        AEMtx33Trans(&pt, mmX + playerPos.x * scaleX, mmY + playerPos.y * scaleY);
        AEMtx33Concat(&pf, &pt, &ps);
        AEGfxSetTransform(pf.m);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        DrawMinimapArrow(mmX, mmY, scaleX, scaleY);

        // Minimap border
        AEMtx33 bS, bT, bF;
        AEMtx33Scale(&bS, minimapWidth, minimapHeight);
        AEMtx33Trans(&bT, mmX, mmY);
        AEMtx33Concat(&bF, &bT, &bS);
        AEGfxSetTransform(bF.m);
        AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);
    }
}

// =====================
// --- GameState Methods ---
// =====================
void GameState::LoadState() {
    bgm.Init(); bgm.PlayNormal();
    halfMapWidth = mapWidth * 0.5f;
    halfMapHeight = mapHeight * 0.5f;
    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);

    // Border
    AEGfxMeshStart();
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    borderMesh = AEGfxMeshEnd();

    tileWorldSizeX = mapWidth / (float)FOG_GRID_SIZE;
    tileWorldSizeY = mapHeight / (float)FOG_GRID_SIZE;

    // Wall mesh
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, 0.5f, 0xFFFFFFFF, 0, 0, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    wallMesh = AEGfxMeshEnd();

    // Fog mesh
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, 0.5f, 0xFFFFFFFF, 0, 0, -0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    fogTileMesh = AEGfxMeshEnd();
}

void GameState::InitState() {
    InitTutorial(currentLevel); 
    playerPos = currentLevel.startPos;
    camPos = playerPos; camVel = { 0,0 };

    for (int i = 0; i < FOG_GRID_SIZE; i++)
        for (int j = 0; j < FOG_GRID_SIZE; j++) {
            discoveryGrid[i][j] = 0;
            regenTimerGrid[i][j] = 0;
        }
}

void GameState::Update(double dt) {
    AEVec2 move = { 0,0 };
    if (AEInputCheckCurr(AEVK_W)) move.y += 1;
    if (AEInputCheckCurr(AEVK_S)) move.y -= 1;
    if (AEInputCheckCurr(AEVK_A)) move.x -= 1;
    if (AEInputCheckCurr(AEVK_D)) move.x += 1;

    if (AEVec2Length(&move) > 0) {
        AEVec2Normalize(&move, &move);
        playerDir = move;
        float step = playerSpeed * (float)dt;
        int subSteps = 3;
        float subStep = step / (float)subSteps;

        for (int i = 0; i < subSteps; i++) {
            AEVec2 nextPosX = { playerPos.x + move.x * subStep, playerPos.y };
            bool collisionX = false;
            for (const auto& w : currentLevel.walls) {
                float safeW = (w.width < 40) ? 40 : w.width;
                float safeH = (w.height < 40) ? 40 : w.height;
                if (CircleRectCollision(w.position, { safeW,safeH }, nextPosX, playerRadius)) { collisionX = true; break; }
            }
            if (!collisionX) playerPos.x = nextPosX.x;

            AEVec2 nextPosY = { playerPos.x, playerPos.y + move.y * subStep };
            bool collisionY = false;
            for (const auto& w : currentLevel.walls) {
                float safeW = (w.width < 40) ? 40 : w.width;
                float safeH = (w.height < 40) ? 40 : w.height;
                if (CircleRectCollision(w.position, { safeW,safeH }, nextPosY, playerRadius)) { collisionY = true; break; }
            }
            if (!collisionY) playerPos.y = nextPosY.y;
        }
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
    if (wallMesh) AEGfxMeshFree(wallMesh);
    bgm.Exit();
}
