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
#include "../Pets/PetManager.h"
#include "../helpers/CollisionUtils.h"
#include "../Map.h"
#include <iostream>
#include <cmath>
#include "../Actor/Player.h"
#include "../Actor/Enemy.h"
#include "../GameObjects/GameObjectManager.h"
#include "../Helpers/BitmaskUtils.h"
#include "../TutorialData.h"
#include "../GameDB.h"
#include "../TileMap.h"
#include "../Minimap.h"
#define TUTORIAL 0

namespace {
    // --- GLOBAL SYSTEMS ---
    AEGfxVertexList* circleMesh = nullptr;   // Used for Player, Enemies, Boss
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
    Enemy* boss, * enemy1, * enemy2;

    // --- CAMERA DATA ---
    AEVec2 camPos, camVel;
    float camZoom = 1.2f;
    float camSmoothTime = 0.15f;

    // --- MAP SETTINGS ---
    float mapWidth = 2400.0f;
    float mapHeight = 2000.0f;
    float halfMapWidth, halfMapHeight;
    MapData currentLevel;
    TileMap* map{};
    TileMap* nextMap{};
    Minimap* minimap{};

    // --- TUTORIAL ---
    Tutorial::TutorialFairy* fairy;
    s8 font{};

    //Boss HP & BossProgress bar
    float bossHPProgressBarWidth = 0.f;
    float bossHPProgressBarHeight = 0.f;
    float bossHPProgressBar = 100.f;
    float bossMaxHPProgressBar = 100.f;

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
        // To be replace boss logic
        if (!bossAlive) {
            DrawTintedMesh(GetTransformMtx({ currentLevel.doorPos.x - 335.0f, currentLevel.doorPos.y }, 0, { 45,125 }),
                wallMesh, nullptr, { 0, 0.8f * 255, 0, 255 }, 255);
        }

        map->Render();
        if (nextMap) nextMap->Render(); // Render the procedural map next to the CSV map
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
        AEVec2 bhpbPos = ToVec2(AEGfxGetWinMinX() + bhpbSize.x / 2 + bhpbMargin / 2, barY);

        AEMtx33 bhpbMatrix;
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);
        GetTransformMtx(bhpbMatrix, bgPos, 0, bgSize);
        AEGfxSetTransform(bhpbMatrix.m);
        AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        GetTransformMtx(bhpbMatrix, bhpbPos, 0, bhpbSize);
        AEGfxSetTransform(bhpbMatrix.m);
        (bossAlive) ? AEGfxSetColorToMultiply(0.7f, 0.2f, 0.2f, 1.f) : AEGfxSetColorToMultiply(0.9f, 0.9f, 0.9f, 1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }
}

void GameState::LoadState() {
    if (!GameDB::LoadEnemyDefs("Assets/Data/enemies.json")) {
        std::cout << "WARNING: enemies.json failed to load.\n";
    }

    font = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 72);
    bgm.Init();
    bgm.PlayNormal();

    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);
    squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

    // 1. Create the map first with your preferred tile scale
    map = new TileMap("Assets/map.csv", { 0,0 }, 115.f, 115.f);

    // Bridge logic: Open the last column of the CSV map to connect to nextMap
    unsigned csvRows = map->GetMapSize().second;
    unsigned csvCols = map->GetMapSize().first;
    map->GetMutableTileData()[csvRows / 2][csvCols - 1] = TileMap::TILE_NONE;

    // 2. Initialize nextMap at the offset of the first map's width
    AEVec2 proceduralOffset = { map->GetFullMapSize().x, 0 };
    nextMap = new TileMap(proceduralOffset, 115.f, 115.f);
    nextMap->GenerateProcedural(50, 50, 1234);

    // Keep your minimap as it is
    minimap = new Minimap{};

    // 3. Update global map dimensions to encompass BOTH maps
    mapWidth = map->GetFullMapSize().x + nextMap->GetFullMapSize().x;
    mapHeight = map->GetFullMapSize().y;
    halfMapWidth = mapWidth * 0.5f;
    halfMapHeight = mapHeight * 0.5f;

    AEVec2Zero(&playerPos);
    playerDir = { 1.0f, 0.0f };

    AEVec2Zero(&camPos);
    AEVec2Zero(&camVel);

    // 4. Update the collision grid with the CORRECT combined dimensions
    GameObjectManager::GetInstance()->InitCollisionGrid(
        static_cast<unsigned>(mapWidth),
        static_cast<unsigned>(mapHeight)
    );

    // --- Spawn player GO once ---
    if (!gPlayer) gPlayer = new Player();
    PetManager::GetInstance()->LinkPlayer(gPlayer);

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

    AEVec2 safeSpawnPos = map->GetTilePosition(2, 2);
    // Player collides with pickups + enemies
    Bitmask collideMask = CreateBitmask(3,
        Collision::LAYER::ENEMIES,
        Collision::LAYER::INTERACTABLE,
        Collision::LAYER::OBSTACLE
    );

    gPlayer->Init(
        safeSpawnPos,
        AEVec2{ playerRadius * 2.f, playerRadius * 2.f },
        0,
        MESH_CIRCLE,
        Collision::SHAPE::COL_CIRCLE,
        AEVec2{ playerRadius * 2.f, playerRadius * 2.f },
        collideMask,
        Collision::LAYER::PLAYER
    );
    PetManager::GetInstance()->PlacePet(GetPlayerPos());
    PetManager::GetInstance()->InitPetForGame();

    ActorStats base{};
    base.maxHP = 100.0f;
    base.attack = 10.0f;
    base.attackSpeed = 1.0f;
    base.moveSpeed = playerSpeed;
    gPlayer->InitPlayerRuntime(base);

    //Init other gameobjects
    LootChest* chest = dynamic_cast<LootChest*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
    chest->Init(currentLevel.chestPos, { 35,35 }, 0, MESH_SQUARE, Collision::COL_RECT, { 35,35 }, CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE)
        ->GetRenderData().tint = CreateColor(255, 0.84f * 255.f, 0, 255);

    const EnemyDef* bossDef = GameDB::GetEnemyDef(2);
    const EnemyDef* slimeDef = GameDB::GetEnemyDef(1);

    std::cout << "bossDef=" << (bossDef ? "OK" : "NULL")
        << " slimeDef=" << (slimeDef ? "OK" : "NULL") << "\n";

    auto SpawnEnemyFromDef = [&](Enemy* enemy, const EnemyDef* def, AEVec2 pos)
        {
            if (!enemy || !def) return;

            float r = def->render.radius;

            MESH_SHAPE meshShape = (def->render.mesh == EnemyMesh::Square) ? MESH_SQUARE : MESH_CIRCLE;
            Collision::SHAPE colShape = (meshShape == MESH_SQUARE) ? Collision::COL_RECT : Collision::COL_CIRCLE;

            enemy->Init(
                pos,
                { r * 2.0f, r * 2.0f },
                0,
                meshShape,
                colShape,
                { r * 2.0f, r * 2.0f },
                CreateBitmask(2, Collision::PLAYER, Collision::OBSTACLE),
                Collision::ENEMIES
            )->GetRenderData().tint = def->render.tint;

            enemy->InitEnemyRuntime(def);
        };

    SpawnEnemyFromDef(enemy1, slimeDef, currentLevel.enemy1Pos);
    SpawnEnemyFromDef(enemy2, slimeDef, currentLevel.enemy2Pos);
    SpawnEnemyFromDef(boss, bossDef, currentLevel.doorPos);


    // Camera starts on player
    camPos = safeSpawnPos;
    camVel = { 0,0 };

    minimap->Reset();

    if (TUTORIAL) {
        fairy->InitTutorial(gPlayer, &currentLevel, { enemy1, enemy2, boss });
    }
}

void GameState::Update(double dt)
{
    if (AEInputCheckTriggered(AEVK_M)) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
        return; // Exit the update early since we are switching states
    }
#pragma region inputs_for_testing
    //Press L to spawn test chest at the mouse location
    if (AEInputCheckTriggered(AEVK_L)) {
        LootChest* chest = dynamic_cast<LootChest*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
        AEVec2 m = GetMouseWorldVec();
        chest->Init(m, { 75,75 }, 1, MESH_SQUARE, Collision::COL_RECT, { 75,75 }, CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE);
    }

    //Pet skill test. check cout
    if (AEInputCheckTriggered(AEVK_R)) {
        PostOffice::GetInstance()->Send("PetManager", new PetSkillMsg(PetSkillMsg::CAST_SKILL));
    }
#pragma endregion

    if (!gPlayer) return;

    if (nextMap && gPlayer->GetPos().x > nextMap->GetFullMapSize().x * 0.5f + nextMap->GetOffset().x - (map->GetFullMapSize().x * 0.5f)) {

        AEVec2 newOffset = { nextMap->GetOffset().x + nextMap->GetFullMapSize().x, 0 };
        map->SetOffset(newOffset);
        map->GenerateProcedural(50, 50, rand());

        TileMap* temp = map;
        map = nextMap;
        nextMap = temp;
        mapWidth = map->GetFullMapSize().x + nextMap->GetFullMapSize().x;
        halfMapWidth = mapWidth * 0.5f;
    }

    AEVec2 move = gPlayer->GetMoveDirNorm();
    f32 len = AEVec2Length(&move);
    if (len > 0 || gPlayer->HasForceApplied()) {
        playerDir = gPlayer->GetMoveDirNorm();
    }

    //Set hp bar to follow boss hp
    bossHPProgressBar = (boss->GetHP() / boss->GetMaxHP()) * bossMaxHPProgressBar;

    bossAlive = !boss->IsDead();
    if (TUTORIAL && fairy->data.stage == Tutorial::BOSS && !bossAlive) {
        fairy->ChangeStage(Tutorial::END);
    }

    // Determine which map the player is currently inside for accurate collisions
    TileMap* currentMap = (gPlayer->GetPos().x > nextMap->GetOffset().x) ? nextMap : map;

    // Systems now read from gPlayer via GetPlayerPos()
    minimap->Update(dt, *currentMap, *gPlayer);
    UpdateWorldMap((float)dt);

    // Pass the current active map to the GameObjectManager for collision checks
    GameObjectManager::GetInstance()->UpdateObjects(dt, currentMap);
}

void GameState::Draw() {
    RenderWorldMap();
    GameObjectManager::GetInstance()->DrawObjects();
    DrawBossHPProgressBar();
    minimap->Render(*map, *gPlayer);

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
    if (wallMesh) AEGfxMeshFree(wallMesh);
    delete map;
    delete nextMap; 
    delete minimap;
    bgm.Exit();
    if (font >= 0)
        AEGfxDestroyFont(font);
}

// Boss getters and setters

bool getBossAlive() {
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