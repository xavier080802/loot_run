#include "../GameStates/GameState.h"
#include "../Music.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/CoordUtils.h"
#include "../camera.h"
#include "../RenderingManager.h"
#include "../GameObjects/GameObject.h"
#include "../Helpers/CollisionUtils.h"
#include "../Map.h"
#include <iostream>
#include <cmath>
#include "../Actor/Player.h"
#include "../GameObjects/GameObjectManager.h"
#include "../Helpers/BitmaskUtils.h"

namespace {
    // --- GLOBAL SYSTEMS ---
    BGMManager bgm;
    AEGfxVertexList* circleMesh = nullptr;   // Used for Player, Enemies, Boss
    AEGfxVertexList* borderMesh = nullptr;   // Used for Minimap frame
    AEGfxVertexList* fogTileMesh = nullptr; // Used for Fog grid squares
    AEGfxVertexList* wallMesh = nullptr;    // Used for Walls, Doors, Chests

    // --- PLAYER DATA ---
    Player* gPlayer = nullptr;
    AEVec2 playerDir = { 1.0f, 0.0f };
    float playerRadius = 15.0f;
    float playerSpeed = 300.0f;

    static AEVec2 GetPlayerPos()
    {
        return gPlayer ? gPlayer->GetPos() : AEVec2{ 0.0f, 0.0f };
    }

    // --- BOSS PLACEHOLDER ---
    bool bossAlive = true;
    float bossRadius = 60.0f;

    // --- CAMERA DATA ---
    AEVec2 camPos, camVel;
    float camZoom = 1.2f;
    float camSmoothTime = 0.15f;

    // --- MAP SETTINGS ---
    float mapWidth = 2400.0f;
    float mapHeight = 2000.0f;
    float halfMapWidth, halfMapHeight;
    MapData currentLevel;

    // --- FOG OF WAR SETTINGS ---
    const int FOG_GRID_SIZE = 40;
    float discoveryGrid[FOG_GRID_SIZE][FOG_GRID_SIZE];      // 1.0 = Visible, 0.0 = Dark
    float regenTimerGrid[FOG_GRID_SIZE][FOG_GRID_SIZE];     // Seconds before fog returns
    const float REGEN_DELAY = 5.0f;                         // Stay revealed for 5s
    const float FOG_REGEN_RATE = 0.3f;                      // How fast it fades back to dark
    float tileWorldSizeX, tileWorldSizeY;

    // --- MINIMAP UI SETTINGS ---
    float minimapWidth = 200.0f;
    float minimapHeight = 200.0f;
    float minimapMargin = 20.0f;
    float minimapPlayerScaleFactor = 3.5f;

    // Logic to calculate which fog tiles are near the player and reveal them
    void UpdateDiscovery(float dt) {
        AEVec2 p = GetPlayerPos();
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

                float dx = p.x - tileWorldX;
                float dy = p.y - tileWorldY;

                if ((dx * dx + dy * dy) < (revealRadiusWorld * revealRadiusWorld)) {
                    discoveryGrid[x][y] = 1.0f;
                    regenTimerGrid[x][y] = REGEN_DELAY;
                }
            }
        }
    }

    // Draws the small orientation arrow on the minimap
    void DrawMinimapArrow(float mmX, float mmY, float scaleX, float scaleY) {
        AEVec2 p = GetPlayerPos();
        float cx = mmX + p.x * scaleX;
        float cy = mmY + p.y * scaleY;

        float dx = playerDir.x, dy = playerDir.y;
        float mag = sqrtf(dx * dx + dy * dy);
        if (mag < 1e-4f) { dx = 1.0f; dy = 0.0f; }
        else { dx /= mag; dy /= mag; }

        float px = -dy, py = dx; // Perpendicular vector for width
        float dotSize = playerRadius * scaleX * minimapPlayerScaleFactor;

        // Offset arrow slightly in front of the player dot
        float ax = cx + dx * (dotSize + 2.0f), ay = cy + dy * (dotSize + 2.0f);
        float tipX = ax + dx * 8.0f, tipY = ay + dy * 8.0f;
        float bLX = ax + px * 4.0f, bLY = ay + py * 4.0f;
        float bRX = ax - px * 4.0f, bRY = ay - py * 4.0f;

        AEGfxMeshStart();
        AEGfxTriAdd(tipX, tipY, 0xFF00FFFF, 0, 0, bLX, bLY, 0xFF00FFFF, 0, 0, bRX, bRY, 0xFF00FFFF, 0, 0);
        AEGfxVertexList* arrowMesh = AEGfxMeshEnd();

        AEMtx33 identity; AEMtx33Identity(&identity);
        AEGfxSetTransform(identity.m);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxMeshDraw(arrowMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxMeshFree(arrowMesh);
    }

    // Handles Camera movement and smooth damping
    void UpdateWorldMap(float dt) {
        float winW = (float)AEGfxGetWinMaxX(), winH = (float)AEGfxGetWinMaxY();
        float viewHalfW = (winW * 0.5f) / camZoom, viewHalfH = (winH * 0.5f) / camZoom;

        AEVec2 camTarget = GetPlayerPos();
        float limitX = halfMapWidth - viewHalfW, limitY = halfMapHeight - viewHalfH;

        // Keep camera within map bounds
        if (limitX > 0) camTarget.x = AEClamp(camTarget.x, -limitX, limitX); else camTarget.x = 0;
        if (limitY > 0) camTarget.y = AEClamp(camTarget.y, -limitY, limitY); else camTarget.y = 0;

        // Critically damped spring for smooth camera movement
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

        // Camera matrix math
        AEMtx33 viewMtx, zoomMtx, camMatrix;
        AEMtx33Trans(&viewMtx, -camPos.x, -camPos.y);
        AEMtx33Scale(&zoomMtx, camZoom, camZoom);
        AEMtx33Concat(&camMatrix, &zoomMtx, &viewMtx);

        // --- WORLD OBJECTS RENDERING ---
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        for (const auto& wall : currentLevel.walls) {
            AEMtx33 s, t, final;
            AEMtx33Scale(&s, wall.width + 4.0f, wall.height + 4.0f);
            AEMtx33Trans(&t, std::floor(wall.position.x), std::floor(wall.position.y));
            AEMtx33Concat(&final, &camMatrix, &t);
            AEMtx33Concat(&final, &final, &s);
            AEGfxSetTransform(final.m);
            AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
        }

        // To be replace boss logic
        if (bossAlive) {
            //placeholder boss
            AEMtx33 bS, bT, bF; AEMtx33Scale(&bS, bossRadius * 2, bossRadius * 2); AEMtx33Trans(&bT, currentLevel.doorPos.x, currentLevel.doorPos.y);
            AEMtx33Concat(&bF, &camMatrix, &bT); AEMtx33Concat(&bF, &bF, &bS); AEGfxSetTransform(bF.m);
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

            // World Door Rect 
            AEMtx33 dS, dT, dF; AEMtx33Scale(&dS, 45.0f, 125.0f); AEMtx33Trans(&dT, currentLevel.doorPos.x - 335.0f, currentLevel.doorPos.y);
            AEMtx33Concat(&dF, &camMatrix, &dT); AEMtx33Concat(&dF, &dF, &dS); AEGfxSetTransform(dF.m);
            AEGfxSetColorToMultiply(0.0f, 0.8f, 0.0f, 1.0f); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
        }

        // Placeholder Chest
        AEMtx33 cS, cT, cF; AEMtx33Scale(&cS, 35, 35); AEMtx33Trans(&cT, currentLevel.chestPos.x, currentLevel.chestPos.y);
        AEMtx33Concat(&cF, &camMatrix, &cT); AEMtx33Concat(&cF, &cF, &cS); AEGfxSetTransform(cF.m);
        AEGfxSetColorToMultiply(1.0f, 0.84f, 0.0f, 1.0f); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);

        // Placeholder enemies
        AEVec2 enemies[] = { currentLevel.enemy1Pos, currentLevel.enemy2Pos };
        for (auto& e : enemies) {
            AEMtx33 eS, eT, eF; AEMtx33Scale(&eS, 30, 30); AEMtx33Trans(&eT, e.x, e.y);
            AEMtx33Concat(&eF, &camMatrix, &eT); AEMtx33Concat(&eF, &eF, &eS); AEGfxSetTransform(eF.m);
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
        }

        // Player
        AEVec2 p = GetPlayerPos();
        AEMtx33 pS, pT, pFinal; AEMtx33Scale(&pS, playerRadius * 2, playerRadius * 2); AEMtx33Trans(&pT, p.x, p.y);
        AEMtx33Concat(&pFinal, &camMatrix, &pT); AEMtx33Concat(&pFinal, &pFinal, &pS); AEGfxSetTransform(pFinal.m);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

        // --- MINIMAP RENDERING LAYERS ---
        float scaleX = minimapWidth / mapWidth, scaleY = minimapHeight / mapHeight;
        float mmX = (float)AEGfxGetWinMaxX() - minimapWidth * 0.5f - minimapMargin;
        float mmY = (float)AEGfxGetWinMaxY() - minimapHeight * 0.5f - minimapMargin;
        float playerMinimapRadius = playerRadius * scaleX * minimapPlayerScaleFactor;

        // LAYER A: Minimap Walls 
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        for (const auto& w : currentLevel.walls) {
            AEMtx33 s, t, f; AEMtx33Scale(&s, (w.width + 4.0f) * scaleX, (w.height + 4.0f) * scaleY);
            AEMtx33Trans(&t, mmX + w.position.x * scaleX, mmY + w.position.y * scaleY);
            AEMtx33Concat(&f, &t, &s); AEGfxSetTransform(f.m); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
        }

        // LAYER B: Minimap Quest Icons & Minimap Boss
        if (bossAlive) {
            AEMtx33 bms, bmt, bmf; AEMtx33Scale(&bms, playerMinimapRadius * 2, playerMinimapRadius * 2);
            AEMtx33Trans(&bmt, mmX + currentLevel.doorPos.x * scaleX, mmY + currentLevel.doorPos.y * scaleY);
            AEMtx33Concat(&bmf, &bmt, &bms); AEGfxSetTransform(bmf.m);
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

            // Minimap Door
            AEMtx33 dms, dmt, dmf; AEMtx33Scale(&dms, playerMinimapRadius, playerMinimapRadius * 2.5f);
            AEMtx33Trans(&dmt, mmX + (currentLevel.doorPos.x - 335.0f) * scaleX, mmY + currentLevel.doorPos.y * scaleY);
            AEMtx33Concat(&dmf, &dmt, &dms); AEGfxSetTransform(dmf.m);
            AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
        }

        //  Minimap Chest
        AEMtx33 mcs, mct, mcf; AEMtx33Scale(&mcs, playerMinimapRadius * 2, playerMinimapRadius * 2);
        AEMtx33Trans(&mct, mmX + currentLevel.chestPos.x * scaleX, mmY + currentLevel.chestPos.y * scaleY);
        AEMtx33Concat(&mcf, &mct, &mcs); AEGfxSetTransform(mcf.m);
        AEGfxSetColorToMultiply(1.0f, 0.84f, 0.0f, 1.0f); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);

        // Minimap Enemies
        for (auto& e : enemies) {
            AEMtx33 es, et, ef; AEMtx33Scale(&es, playerMinimapRadius * 2, playerMinimapRadius * 2);
            AEMtx33Trans(&et, mmX + e.x * scaleX, mmY + e.y * scaleY);
            AEMtx33Concat(&ef, &et, &es); AEGfxSetTransform(ef.m);
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
        }

        // LAYER C: Fog Grid 
        for (int x = 0; x < FOG_GRID_SIZE; x++) {
            for (int y = 0; y < FOG_GRID_SIZE; y++) {
                if (discoveryGrid[x][y] < 1.0f) {
                    AEMtx33 s, t, f; AEMtx33Scale(&s, (tileWorldSizeX + 4.0f) * scaleX, (tileWorldSizeY + 4.0f) * scaleY);
                    float oX = (x * tileWorldSizeX) - halfMapWidth + tileWorldSizeX * 0.5f;
                    float oY = (y * tileWorldSizeY) - halfMapHeight + tileWorldSizeY * 0.5f;
                    AEMtx33Trans(&t, mmX + oX * scaleX, mmY + oY * scaleY);
                    AEMtx33Concat(&f, &t, &s); AEGfxSetTransform(f.m);
                    // Use discoveryGrid value for Alpha (0.0 discovery = 1.0 alpha/darkness)
                    AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 1.0f - discoveryGrid[x][y]);
                    AEGfxMeshDraw(fogTileMesh, AE_GFX_MDM_TRIANGLES);
                }
            }
        }

        // LAYER D: Player & UI Border 
        AEMtx33 ps, pt, pf; AEMtx33Scale(&ps, playerMinimapRadius * 2, playerMinimapRadius * 2);
        AEMtx33Trans(&pt, mmX + p.x * scaleX, mmY + p.y * scaleY);
        AEMtx33Concat(&pf, &pt, &ps); AEGfxSetTransform(pf.m);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
        DrawMinimapArrow(mmX, mmY, scaleX, scaleY);

        // Minimap frame
        AEMtx33 bS, bT, bF; AEMtx33Scale(&bS, minimapWidth, minimapHeight); AEMtx33Trans(&bT, mmX, mmY);
        AEMtx33Concat(&bF, &bT, &bS); AEGfxSetTransform(bF.m);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);
    }
}

void GameState::LoadState() {
    bgm.Init(); bgm.PlayNormal();
    halfMapWidth = mapWidth * 0.5f; halfMapHeight = mapHeight * 0.5f;
    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);

    // Initialise the 4-point border mesh for the UI
    AEGfxMeshStart();
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0); AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 0, 0); AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0); borderMesh = AEGfxMeshEnd();

    tileWorldSizeX = mapWidth / (float)FOG_GRID_SIZE; tileWorldSizeY = mapHeight / (float)FOG_GRID_SIZE;

    // Initialise the square mesh used for walls and fog tiles
    AEGfxMeshStart();
    AEGfxTriAdd(-0.52f, -0.52f, 0xFFFFFFFF, 0, 0, 0.52f, -0.52f, 0xFFFFFFFF, 0, 0, 0.52f, 0.52f, 0xFFFFFFFF, 0, 0);
    AEGfxTriAdd(-0.52f, -0.52f, 0xFFFFFFFF, 0, 0, 0.52f, 0.52f, 0xFFFFFFFF, 0, 0, -0.52f, 0.52f, 0xFFFFFFFF, 0, 0);
    wallMesh = AEGfxMeshEnd();
    fogTileMesh = wallMesh;
}

void GameState::InitState()
{
    InitTutorial(currentLevel);

    // --- Spawn player GO once ---
    if (!gPlayer) gPlayer = new Player();

    // Player collides with pickups (PET) + enemies
    Bitmask collideMask = CreateBitmask(2,
        (int)GameObject::COLLISION_LAYER::PET,
        (int)GameObject::COLLISION_LAYER::ENEMIES
    );

    gPlayer->Init(
        currentLevel.startPos,
        AEVec2{ 1.0f, 1.0f },
        0,
        MESH_CIRCLE,
        COLLIDER_SHAPE::COL_CIRCLE,
        AEVec2{ playerRadius * 2.0f, playerRadius * 2.0f },
        collideMask,
        GameObject::COLLISION_LAYER::PLAYER
    );

    ActorStats base{};
    base.maxHP = 100.0f;
    base.moveSpeed = playerSpeed;
    gPlayer->InitPlayerRuntime(base);

    // Camera starts on player
    camPos = currentLevel.startPos;
    camVel = { 0,0 };

    // Start with a fully hidden map
    for (int i = 0; i < FOG_GRID_SIZE; i++)
        for (int j = 0; j < FOG_GRID_SIZE; j++) {
            discoveryGrid[i][j] = 0;
            regenTimerGrid[i][j] = 0;
        }
}

void GameState::Update(double dt)
{
    // Track input direction for minimap arrow (Player does the actual movement)
    AEVec2 move = { 0,0 };
    if (AEInputCheckCurr(AEVK_W)) move.y += 1;
    if (AEInputCheckCurr(AEVK_S)) move.y -= 1;
    if (AEInputCheckCurr(AEVK_A)) move.x -= 1;
    if (AEInputCheckCurr(AEVK_D)) move.x += 1;

    if (AEVec2Length(&move) > 0.0f) {
        AEVec2Normalize(&move, &move);
        playerDir = move;
    }

    if (!gPlayer) return;

    // Run player (movement + pickup collision callbacks etc.)
    AEVec2 oldPos = gPlayer->GetPos();
    gPlayer->Update(dt);
    AEVec2 newPos = gPlayer->GetPos();

    // World collision test (walls + boss door)
    auto IsClear = [&](AEVec2 p) {
        for (const auto& w : currentLevel.walls)
            if (CircleRectCollision(w.position, { w.width, w.height }, p, playerRadius))
                return false;

        if (bossAlive) {
            if (AEVec2Distance(&p, &currentLevel.doorPos) < (playerRadius + bossRadius))
                return false;


            if (CircleRectCollision({ currentLevel.doorPos.x - 335.0f, currentLevel.doorPos.y },
                { 40, 120 }, p, playerRadius))
                return false;
        }

        return true;
        };

    // If invalid, resolve by sub-stepping the attempted delta (same style as your old code)
    if (!IsClear(newPos)) {
        AEVec2 delta = { newPos.x - oldPos.x, newPos.y - oldPos.y };

        const int steps = 10;
        AEVec2 pos = oldPos;

        for (int i = 0; i < steps; i++) {
            AEVec2 nextX = { pos.x + delta.x / steps, pos.y };
            if (IsClear(nextX)) pos.x = nextX.x;


            AEVec2 nextY = { pos.x, pos.y + delta.y / steps };
            if (IsClear(nextY)) pos.y = nextY.y;
        }

        gPlayer->SetPos(pos);
        newPos = pos;
    }

    // Clamp to world bounds
    newPos.x = AEClamp(newPos.x, -halfMapWidth + playerRadius, halfMapWidth - playerRadius);
    newPos.y = AEClamp(newPos.y, -halfMapHeight + playerRadius, halfMapHeight - playerRadius);
    gPlayer->SetPos(newPos);


    // Systems now read from gPlayer via GetPlayerPos()
    UpdateDiscovery((float)dt);
    UpdateWorldMap((float)dt);
}

void GameState::Draw() { RenderWorldMap(); }
void GameState::ExitState() {}

void GameState::UnloadState() {
    if (borderMesh) AEGfxMeshFree(borderMesh);
    if (wallMesh) AEGfxMeshFree(wallMesh);
    bgm.Exit();
}