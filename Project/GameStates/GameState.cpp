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
#include "../UI/Minimap.h"
#include "../Drops/DropSystem.h"

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
    Enemy* boss;
    std::vector<Enemy*> csvEnemies;

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
    float teleportCooldown = 0.f;
    bool inProceduralMap = false;

    // --- TUTORIAL ---
    bool doTutorial{ false };
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
        if (!bossAlive) {
            DrawTintedMesh(GetTransformMtx({ currentLevel.doorPos.x - 335.0f, currentLevel.doorPos.y }, 0, { 45,125 }),
                wallMesh, nullptr, { 0, 0.8f * 255, 0, 255 }, 255);
        }

        if (inProceduralMap) {
            if (nextMap) nextMap->Render();
        }
        else {
            map->Render();
        }
    }

    // Draws the boss HP BossProgress bar at the top of the screen
    void DrawBossHPProgressBar()
    {
        if (doTutorial && fairy->data.stage != Tutorial::BOSS) return; //Only show hp bar at tut boss stage.

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
    font = RenderingManager::GetInstance()->GetFont();

    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Melee/melee.json", EquipmentCategory::Melee);
    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Range/ranged.json", EquipmentCategory::Ranged);
    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Armor/Head/head.json", EquipmentCategory::Head);
    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Armor/Body/body.json", EquipmentCategory::Body);
    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Armor/Hands/hands.json", EquipmentCategory::Hands);
    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Armor/Feet/feet.json", EquipmentCategory::Feet);

    if (!GameDB::LoadPlayerDef("Assets/Data/Player/player.json"))
    {
        std::cout << "WARNING: player.json failed to load.\n";
    }
    if (!GameDB::LoadPlayerInventory("Assets/Data/Player/inventory.json"))
    {
        std::cout << "WARNING: inventory.json failed to load.\n";
    }
    bgm.Init(); bgm.PlayNormal();
    halfMapWidth = mapWidth * 0.5f; halfMapHeight = mapHeight * 0.5f;
    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);
    squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

    map = new TileMap("Assets/Dungeon.csv", { 0,0 }, 115.f, 115.f);

    float procTileSize = 115.f;
    unsigned procRows = 50, procCols = 50;
    nextMap = new TileMap({ 0.f, 0.f }, procTileSize, procTileSize);
    srand(1234);
    nextMap->GenerateProcedural(procRows, procCols, 1234);

    unsigned csvCols = map->GetMapSize().first;
    unsigned csvRows = map->GetMapSize().second;
    playerPos = map->GetTilePosition(1, 1);
    bool foundSpawn = false;
    for (unsigned r = 1; r < csvRows - 1 && !foundSpawn; ++r) {
        for (unsigned c = 1; c < csvCols - 1 && !foundSpawn; ++c) {
            const TileMap::Tile* t = map->QueryTile(r, c);
            if (t && !t->isSolid && t->type == TileMap::TILE_NONE) {
                playerPos = map->GetTilePosition(r, c);
                foundSpawn = true;
            }
        }
    }

    camPos = playerPos;

    minimap = new Minimap{};

    mapWidth = map->GetFullMapSize().x + nextMap->GetFullMapSize().x;
    mapHeight = (map->GetFullMapSize().y > nextMap->GetFullMapSize().y) ?
        map->GetFullMapSize().y : nextMap->GetFullMapSize().y;

    GameObjectManager::GetInstance()->InitCollisionGrid(
        static_cast<unsigned>(mapWidth),
        static_cast<unsigned>(mapHeight)
    );

    if (!gPlayer) gPlayer = new Player();
    PetManager::GetInstance()->LinkPlayer(gPlayer);

    boss = new Enemy();

    if (doTutorial) {
        fairy = new Tutorial::TutorialFairy();
    }
}

void GameState::InitState()
{
    InitTutorial(currentLevel);

    unsigned csvCols = map->GetMapSize().first;
    unsigned csvRows = map->GetMapSize().second;
    AEVec2 safeSpawnPos = map->GetTilePosition(1, 1);
    bool found = false;
    for (unsigned r = 1; r < csvRows - 1 && !found; ++r) {
        for (unsigned c = 1; c < csvCols - 1 && !found; ++c) {
            const TileMap::Tile* t = map->QueryTile(r, c);
            if (t && !t->isSolid && t->type == TileMap::TILE_NONE) {
                safeSpawnPos = map->GetTilePosition(r, c);
                found = true;
            }
        }
    }

    // Scan CSV map for enemy, boss and chest tile positions
    std::vector<AEVec2> enemyTilePositions;
    AEVec2 chestTilePos = currentLevel.chestPos;
    bool foundChest = false;
    for (unsigned r = 0; r < csvRows; ++r) {
        for (unsigned c = 0; c < csvCols; ++c) {
            const TileMap::Tile* t = map->QueryTile(r, c);
            if (!t) continue;
            if (t->type == TileMap::TILE_ENEMY)
                enemyTilePositions.push_back(map->GetTilePosition(r, c));
            else if (t->type == TileMap::TILE_CHEST && !foundChest) {
                chestTilePos = map->GetTilePosition(r, c);
                foundChest = true;
            }
        }
    }

    AEVec2 bossPos = enemyTilePositions.size() > 2 ? enemyTilePositions[2] : currentLevel.doorPos;
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

    LootChest* chest = dynamic_cast<LootChest*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
    chest->Init(chestTilePos, { 35,35 }, 0, MESH_SQUARE, Collision::COL_RECT, { 35,35 }, CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE)
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
            );
            enemy->InitEnemyRuntime(def);

            // Set texture after InitEnemyRuntime so it doesn't get overwritten
            enemy->GetRenderData().AddTexture("Assets/enemyplaceholder.png");
            enemy->GetRenderData().SetActiveTexture(0);
            enemy->GetRenderData().tint = CreateColor(255, 255, 255, 255);
            enemy->GetRenderData().alpha = 255;
        };

    // Spawn one enemy per TILE_ENEMY found in CSV
    csvEnemies.clear();
    for (size_t i = 0; i < enemyTilePositions.size(); ++i) {
        Enemy* e = new Enemy();
        SpawnEnemyFromDef(e, slimeDef, enemyTilePositions[i]);
        csvEnemies.push_back(e);
    }

    // Spawn boss at first enemy tile if available, else fallback
    SpawnEnemyFromDef(boss, bossDef, bossPos);

    camPos = safeSpawnPos;
    camVel = { 0,0 };

    minimap->Reset();

    if (doTutorial) {
        fairy->InitTutorial(gPlayer, &currentLevel);
    }
}

void GameState::Update(double dt)
{
    if (AEInputCheckTriggered(AEVK_M)) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
        return;
    }
#pragma region inputs_for_testing
    if (AEInputCheckTriggered(AEVK_L)) {
        LootChest* chest = dynamic_cast<LootChest*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
        AEVec2 m = GetMouseWorldVec();
        chest->Init(m, { 75,75 }, 1, MESH_SQUARE, Collision::COL_RECT, { 75,75 }, CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE);
    }

    if (AEInputCheckTriggered(AEVK_R)) {
        PostOffice::GetInstance()->Send("PetManager", new PetSkillMsg(PetSkillMsg::CAST_SKILL));
    }
#pragma endregion

    if (!gPlayer) return;

    if (teleportCooldown > 0.f) teleportCooldown -= (float)dt;

    if (teleportCooldown <= 0.f && nextMap) {
        if (!inProceduralMap && map->IsConnector(gPlayer->GetPos())) {
            nextMap->GenerateProcedural(50, 50, rand());
            AEVec2 procSpawn = nextMap->GetSpawnPoint();
            gPlayer->SetPos(procSpawn);
            gPlayer->Move({ 0,0 });
            camPos = procSpawn;
            camVel = { 0, 0 };
            halfMapWidth = nextMap->GetFullMapSize().x * 0.5f;
            halfMapHeight = nextMap->GetFullMapSize().y * 0.5f;
            SetCameraPos(camPos);
            inProceduralMap = true;
            teleportCooldown = 2.f;
        }
        else if (inProceduralMap && nextMap->IsConnector(gPlayer->GetPos())) {
            nextMap->GenerateProcedural(50, 50, rand());
            AEVec2 procSpawn = nextMap->GetSpawnPoint();
            gPlayer->SetPos(procSpawn);
            gPlayer->Move({ 0,0 });
            camPos = procSpawn;
            camVel = { 0, 0 };
            SetCameraPos(camPos);
            teleportCooldown = 2.f;
        }
    }

    TileMap* currentMap = inProceduralMap ? nextMap : map;

    AEVec2 move = gPlayer->GetMoveDirNorm();
    f32 len = AEVec2Length(&move);
    if (len > 0 || gPlayer->HasForceApplied()) {
        playerDir = gPlayer->GetMoveDirNorm();
    }

    bossHPProgressBar = (boss->GetHP() / boss->GetMaxHP()) * bossMaxHPProgressBar;

    bossAlive = !boss->IsDead();
    if (doTutorial && fairy->data.stage == Tutorial::BOSS && !bossAlive) {
        fairy->ChangeStage(Tutorial::END);
    }
    if (!doTutorial && !bossAlive) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
        std::cout << "BOSS SLAYED\n";
    }

    minimap->Update(dt, *currentMap, *gPlayer);
    UpdateWorldMap((float)dt);

    GameObjectManager::GetInstance()->UpdateObjects(dt, currentMap);

    DropSystem::PrintPickupDisplay(static_cast<float>(dt));
}

void GameState::Draw() {
    RenderWorldMap();
    GameObjectManager::GetInstance()->DrawObjects();
    DrawBossHPProgressBar();
    minimap->Render(*map, *gPlayer);

    if (gPlayer) {
        gPlayer->DrawUI();
    }

    HandleTutorialDialogueRender();

    PetManager::GetInstance()->DrawUI();
}

void GameState::HandleTutorialDialogueRender()
{
    if (!doTutorial || !fairy || !fairy->data.playDialogue) return;
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