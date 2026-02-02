#include "../GameStates/GameState.h"
#include "../Music.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/CoordUtils.h"
#include "../camera.h"
#include "../RenderingManager.h"
#include "../GameObjects/GameObject.h"
#include "../GameObjects/Projectile.h"
#include "../GameObjects/LootChest.h"
#include "../DesignPatterns/PostOffice.h"
#include "../Pets/PetManager.h" //temp
#include "../helpers/CollisionUtils.h"
#include "../Map.h"
#include <iostream>
#include <cmath>
#include "../Actor/Player.h"
#include "../Actor/Enemy.h"
#include "../GameObjects/GameObjectManager.h"
#include "../Helpers/BitmaskUtils.h"
#include "../TutorialData.h"
#define TUTORIAL 1

namespace {
    // --- GLOBAL SYSTEMS ---
    AEGfxVertexList* circleMesh = nullptr;   // Used for Player, Enemies, Boss
    AEGfxVertexList* borderMesh = nullptr;   // Used for Minimap frame
    AEGfxVertexList* fogTileMesh = nullptr; // Used for Fog grid squares
    AEGfxVertexList* wallMesh = nullptr;    // Used for Walls, Doors, Chests
    AEGfxVertexList* squareMesh = nullptr;

    // Player state
    AEVec2 playerPos;
    AEVec2 playerDir = { 1.0f, 0.0f };   // last non-zero movement direction
    // --- PLAYER DATA ---
    Player* gPlayer = nullptr;
    float playerRadius = 15.0f;
    float playerSpeed = 300.0f;

    static AEVec2 GetPlayerPos()
    {
        return gPlayer ? gPlayer->GetPos() : AEVec2{ 0.0f, 0.0f };
    }

    // --- BOSS PLACEHOLDER ---
    bool bossAlive = true;
    float bossRadius = 60.0f;
    Enemy* boss, *enemy1, *enemy2;

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

    // --- TUTORIAL ---
    Tutorial::TutorialFairy* fairy;
    s8 font{};

    //Boss HP & BossProgress bar
	float bossHPProgressBarWidth = 0.f;
	float bossHPProgressBarHeight = 0.f;
    float bossHPProgressBar = 100.f;
    float bossMaxHPProgressBar = 100.f;

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
        if (!bossAlive) {
            //placeholder boss
            /*AEMtx33 bS, bT, bF; AEMtx33Scale(&bS, bossRadius * 2, bossRadius * 2); AEMtx33Trans(&bT, currentLevel.doorPos.x, currentLevel.doorPos.y);
            AEMtx33Concat(&bF, &camMatrix, &bT); AEMtx33Concat(&bF, &bF, &bS); AEGfxSetTransform(bF.m);
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);*/

            // World Door Rect 
            AEMtx33 dS, dT, dF; AEMtx33Scale(&dS, 45.0f, 125.0f); AEMtx33Trans(&dT, currentLevel.doorPos.x - 335.0f, currentLevel.doorPos.y);
            AEMtx33Concat(&dF, &camMatrix, &dT); AEMtx33Concat(&dF, &dF, &dS); AEGfxSetTransform(dF.m);
            AEGfxSetColorToMultiply(0.0f, 0.8f, 0.0f, 1.0f); AEGfxMeshDraw(wallMesh, AE_GFX_MDM_TRIANGLES);
        }

        // Placeholder enemies
        /*AEVec2 enemies[] = { currentLevel.enemy1Pos, currentLevel.enemy2Pos };
        for (auto& e : enemies) {
            AEMtx33 eS, eT, eF; AEMtx33Scale(&eS, 30, 30); AEMtx33Trans(&eT, e.x, e.y);
            AEMtx33Concat(&eF, &camMatrix, &eT); AEMtx33Concat(&eF, &eF, &eS); AEGfxSetTransform(eF.m);
            AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
        }*/
    }

    void RenderMinimap() {
        // --- MINIMAP RENDERING LAYERS ---
        float scaleX = minimapWidth / mapWidth, scaleY = minimapHeight / mapHeight;
        float mmX = (float)AEGfxGetWinMaxX() - minimapWidth * 0.5f - minimapMargin;
        float mmY = (float)AEGfxGetWinMaxY() - minimapHeight * 0.5f - minimapMargin - 50.f;
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
        AEVec2 enemies[] = { currentLevel.enemy1Pos, currentLevel.enemy2Pos }; //Placeholder enemies
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
        AEVec2 p = GetPlayerPos();
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
	// Draws the boss HP BossProgress bar at the top of the screen
    void DrawBossHPProgressBar()
    {
        if (TUTORIAL && fairy->data.stage != Tutorial::BOSS) return; //Only show hp bar at tut boss stage.

        bossHPProgressBarHeight = 50.f;
        bossHPProgressBarWidth = (float)AEGfxGetWinMaxX() - (float)AEGfxGetWinMinX();
        float barX = -bossHPProgressBarWidth * 0.5f;
        float barY = (float)AEGfxGetWinMaxY() - bossHPProgressBarHeight * 0.5f;
        float bhpbMargin = 4.f;
        float hpRatio = bossHPProgressBar / bossMaxHPProgressBar;
        hpRatio = AEClamp(hpRatio, 0.0f, 1.f);

        // Background bar
        AEVec2 bgSize = ToVec2(bossHPProgressBarWidth, bossHPProgressBarHeight);
        AEVec2 bhpbSize = ToVec2((bossHPProgressBarWidth - bhpbMargin) * hpRatio, bossHPProgressBarHeight - bhpbMargin);
        AEVec2 bgPos = ToVec2(barX + bossHPProgressBarWidth * 0.5f, barY);
        AEVec2 bhpbPos = ToVec2(AEGfxGetWinMinX()+bhpbSize.x / 2 + bhpbMargin / 2, barY);
        
        AEMtx33 bhpbMatrix;
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);
        GetTransformMtx(bhpbMatrix, bgPos, 0, bgSize);
        AEGfxSetTransform(bhpbMatrix.m);
        AEGfxSetColorToMultiply(0.3f,0.3f,0.3f,1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        GetTransformMtx(bhpbMatrix, bhpbPos, 0, bhpbSize);
        AEGfxSetTransform(bhpbMatrix.m);
        (bossAlive) ? AEGfxSetColorToMultiply(0.7f, 0.2f, 0.2f, 1.f) : AEGfxSetColorToMultiply(0.9f, 0.9f, 0.9f, 1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }
}

void GameState::LoadState() {
    font = AEGfxCreateFont("Assets/placeholder.ttf", 72);
    bgm.Init(); bgm.PlayNormal();
    halfMapWidth = mapWidth * 0.5f; halfMapHeight = mapHeight * 0.5f;
    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);
    squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

    AEVec2Zero(&playerPos);
    playerDir = { 1.0f, 0.0f };

    AEVec2Zero(&camPos);
    AEVec2Zero(&camVel);

    GameObjectManager::GetInstance()->InitCollisionGrid(static_cast<unsigned>(mapWidth), static_cast<unsigned>(mapHeight));

    playerDir = { 1.0f, 0.0f };

    // --- Spawn player GO once ---
    if (!gPlayer) gPlayer = new Player();
    PetManager::GetInstance()->LinkPlayer(gPlayer);
    
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

    // Load Enemies
    boss = new Enemy();
    enemy1 = new Enemy();
    enemy2 = new Enemy();

    if (TUTORIAL) {
        fairy = new Tutorial::TutorialFairy();
    }
}

void GameState::InitState()
{
    InitTutorial(currentLevel);

    // Player collides with pickups + enemies
    Bitmask collideMask = CreateBitmask(2,
        GameObject::COLLISION_LAYER::ENEMIES,
        GameObject::COLLISION_LAYER::INTERACTABLE
    );

    gPlayer->Init(
        currentLevel.startPos,
        AEVec2{ playerRadius*2, playerRadius*2 },
        0,
        MESH_CIRCLE,
        COLLIDER_SHAPE::COL_CIRCLE,
        AEVec2{ playerRadius * 2.0f, playerRadius * 2.0f },
        collideMask,
        GameObject::COLLISION_LAYER::PLAYER
    );
    PetManager::GetInstance()->PlacePet(GetPlayerPos());
    PetManager::GetInstance()->InitPetForGame();

    ActorStats base{};
    base.maxHP = 100.0f;
    base.moveSpeed = playerSpeed;
    gPlayer->InitPlayerRuntime(base);

    //Init other gameobjects
    LootChest* chest = dynamic_cast<LootChest*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
    chest->Init(currentLevel.chestPos, { 35,35 }, 0, MESH_SQUARE, COL_RECT, { 35,35 }, CreateBitmask(1, GameObject::PLAYER), GameObject::INTERACTABLE)
         ->GetRenderData().tint = CreateColor(255, 0.84f * 255.f, 0, 255);

    Color red = CreateColor(255, 0, 0, 255);
    boss->Init(currentLevel.doorPos, { bossRadius * 2.f, bossRadius * 2.f }, 0, MESH_CIRCLE, COL_CIRCLE, { bossRadius * 2.f, bossRadius * 2.f }, CreateBitmask(1, GameObject::PLAYER), GameObject::ENEMIES)
        ->GetRenderData().tint = red;
    boss->InitEnemyRuntime(new EnemyDef{ 0, {100}, 0 });
    enemy1->Init(currentLevel.enemy1Pos, { 30,30 }, 0, MESH_CIRCLE, COL_CIRCLE, { 30,30 }, CreateBitmask(1, GameObject::PLAYER), GameObject::ENEMIES)
        ->GetRenderData().tint = red;
    enemy1->InitEnemyRuntime(new EnemyDef{ 0, {30}, 0 });
    enemy2->Init(currentLevel.enemy2Pos, { 30,30 }, 0, MESH_CIRCLE, COL_CIRCLE, { 30,30 }, CreateBitmask(1, GameObject::PLAYER), GameObject::ENEMIES)
        ->GetRenderData().tint = red;
    enemy2->InitEnemyRuntime(new EnemyDef{ 0, {30}, 0 });

    // Camera starts on player
    camPos = currentLevel.startPos;
    camVel = { 0,0 };

    // Start with a fully hidden map
    for (int i = 0; i < FOG_GRID_SIZE; i++) {
        for (int j = 0; j < FOG_GRID_SIZE; j++) {
            discoveryGrid[i][j] = 0;
            regenTimerGrid[i][j] = 0;
        }
    }

    if (TUTORIAL) {
        fairy->InitTutorial(gPlayer, &currentLevel);
    }
}

void GameState::Update(double dt)
{
    #pragma region inputs_for_testing
        //Press L to spawn test chest at the mouse location
        if (AEInputCheckTriggered(AEVK_L)) {
            LootChest* chest = dynamic_cast<LootChest*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
            AEVec2 m = GetMouseWorldVec();
            chest->Init(m, { 75,75 }, 1, MESH_SQUARE, COL_RECT, { 75,75 }, CreateBitmask(1, GameObject::PLAYER), GameObject::INTERACTABLE);
        }

        //Pet skill test. check cout
        if (AEInputCheckTriggered(AEVK_R)) {
            PostOffice::GetInstance()->Send("PetManager", new PetSkillMsg(PetSkillMsg::CAST_SKILL));
        }
    #pragma endregion

    if (!gPlayer) return;

    gPlayer->HandleAttackInput(dt);
    
    AEVec2 oldPos = gPlayer->GetPos();
    // Track input direction for minimap arrow (Player does the actual movement)
    gPlayer->HandleMovementInput(dt);
    //Normally inside GO.Update, but then theres no collision with the map for the player.
    gPlayer->Temp_DoVelocityMovement(dt);
    AEVec2 newPos = gPlayer->GetPos();

    AEVec2 move = gPlayer->GetMoveDirNorm();
    f32 len = AEVec2Length(&move);

    if (len > 0 || gPlayer->HasForceApplied()) {
        playerDir = move;
        float subStep = (playerSpeed * (float)dt) / 10.0f;

        // Collision detection lambda
        auto IsClear = [&](AEVec2 p) {
            for (const auto& w : currentLevel.walls) {
                if (CircleRectCollision(w.position, { w.width, w.height }, p, playerRadius)) return false;
            }
            if (bossAlive) {
                //Collision against boss object
                if (AEVec2Distance(&p, &currentLevel.doorPos) < (playerRadius + bossRadius)) return false;
            }
            else if (CircleRectCollision({ currentLevel.doorPos.x - 335.0f, currentLevel.doorPos.y }, { 40, 120 }, p, playerRadius)) return false;
            if (TUTORIAL) {
                if (CircleRectCollision(fairy->GetTutBarrier(), { 200, 200 }, p, playerRadius)) return false;
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
    }

    // Clamp player inside map bounds
    newPos.x = AEClamp(newPos.x, -halfMapWidth + playerRadius, halfMapWidth - playerRadius);
    newPos.y = AEClamp(newPos.y, -halfMapHeight + playerRadius, halfMapHeight - playerRadius);

    //Set hp bar to follow boss hp
    bossHPProgressBar = (boss->GetHP() / boss->GetMaxHP()) * bossMaxHPProgressBar;
	// Boss HP BossProgress bar testing. press 1 to decrease bar, 2 to increase bar
    /*if (AEInputCheckTriggered(AEVK_1))bossHPProgressBar -= 10.f;
    if (AEInputCheckTriggered(AEVK_2))bossHPProgressBar += 10.f;*/
    //bossHPProgressBar = AEClamp(bossHPProgressBar, 0.f, bossMaxHPProgressBar);
    bossAlive = !boss->IsDead();
    if (TUTORIAL && fairy->data.stage == Tutorial::BOSS && !bossAlive) {
        fairy->ChangeStage(Tutorial::END);
    }
    
    gPlayer->SetPos(newPos);

    // Systems now read from gPlayer via GetPlayerPos()
    UpdateDiscovery((float)dt);
    UpdateWorldMap((float)dt);

    GameObjectManager::GetInstance()->UpdateObjects(dt);
}

void GameState::Draw() { 
    RenderWorldMap(); 
    GameObjectManager::GetInstance()->DrawObjects();
    DrawBossHPProgressBar();
    RenderMinimap();

    HandleTutorialDialogueRender();
}

void GameState::HandleTutorialDialogueRender()
{
    if (!TUTORIAL || !fairy || !fairy->data.playDialogue) return;
    //Render text
    DrawAEText(font, fairy->data.dialogueLines[fairy->data.currDialogueLine].c_str(),
        fairy->data.dialoguePos, fairy->data.dialogueSize, CreateColor(238, 128, 238, 255), TEXT_MIDDLE, 1);
}

void GameState::ExitState() {
    gPlayer->ClearStatusEffects();
}

void GameState::UnloadState() {
    if (borderMesh) AEGfxMeshFree(borderMesh);
    if (wallMesh) AEGfxMeshFree(wallMesh);
    bgm.Exit();
    if (font >= 0)
    AEGfxDestroyFont(font);
}

// Boss getters and setters

bool getBossAlive(){
    return bossAlive;
}
float getBossHPProgressBar() {
	return bossHPProgressBar;
}
void setBossHPProgressBar(float current) {
    bossHPProgressBar = current;
}
float getBossMaxHPProgressBar() {
	return bossMaxHPProgressBar;
}
void setBossMaxHPProgressBar(float max) {
    bossMaxHPProgressBar = max;
}
