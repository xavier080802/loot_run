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
#include <sstream>
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
#include "../Debug.h"

namespace {
    // --- GLOBAL SYSTEMS ---
    AEGfxVertexList* circleMesh = nullptr;
    AEGfxVertexList* wallMesh = nullptr;
    AEGfxVertexList* squareMesh = nullptr;

    // --- PLAYER DATA ---
    AEVec2  playerPos;
    AEVec2  playerDir = { 1.0f, 0.0f };
    Player* gPlayer = nullptr;
    float   playerRadius = 15.0f;
    float   playerSpeed = 300.0f;

    static AEVec2 GetPlayerPos()
    {
        return gPlayer ? gPlayer->GetPos() : AEVec2{ 0.0f, 0.0f };
    }

    // --- BOSS / ENEMY TRACKING ---
    bool              bossAlive = true;
    bool              bossSpawned = false;
    float             bossRadius = 60.0f;
    Enemy* boss = nullptr;
    std::vector<Enemy*> csvEnemies;
    std::vector<Enemy*> procEnemies;      // enemies in the current room only
    std::vector<Enemy*> allProcEnemies;   // ALL proc enemies ever spawned — never cleared between rooms

    int   enemiesKilledInRoom = 0;  // total kills displayed on the bar (all rooms combined)
    int   enemiesRequiredForBoss = 0;  // total enemies spawned across all rooms
    float bossSpawnThreshold = 1.0f;

    // Tracks which procEnemies were already counted as dead so we don't double-count.
    // Using the pointer as a unique ID — once an enemy is counted dead it's added here.
    std::vector<Enemy*> countedDeadEnemies;

    // --- CAMERA DATA ---
    AEVec2 camPos, camVel;
    float  camZoom = 1.2f;
    float  camSmoothTime = 0.15f;

    // --- MAP SETTINGS ---
    float    mapWidth = 2400.0f;
    float    mapHeight = 2000.0f;
    float    halfMapWidth, halfMapHeight;
    MapData  currentLevel;
    TileMap* map{};
    TileMap* nextMap{};
    Minimap* minimap{};
    float    teleportCooldown = 0.f;
    bool     inProceduralMap = false;

    // --- TUTORIAL ---
    bool doTutorial{ false };
    Tutorial::TutorialFairy* fairy;
    s8 font{ -1 };

    // --- BOSS HP / PROGRESS BAR ---
    float bossHPProgressBarWidth = 0.f;
    float bossHPProgressBarHeight = 0.f;
    float bossHPProgressBar = 100.f;
    float bossMaxHPProgressBar = 100.f;

    // =========================================================
    // --- DEBUG MODE ---
    // =========================================================
    bool debugMode = false;
    bool showDebugOverlay = false;
    bool showKeybindOverlay = false;
    bool debugGodMode = false;
    bool debugShowStats = false;
    bool debugFreezeEnemies = false;

    DebugContext MakeDebugCtx()
    {
        DebugContext ctx;
        ctx.squareMesh = squareMesh;
        ctx.font = font;
        ctx.gPlayer = gPlayer;
        ctx.showDebugOverlay = showDebugOverlay;
        ctx.showKeybindOverlay = showKeybindOverlay;
        ctx.debugGodMode = debugGodMode;
        ctx.debugShowStats = debugShowStats;
        ctx.debugFreezeEnemies = debugFreezeEnemies;
        ctx.inProceduralMap = inProceduralMap;
        ctx.bossSpawned = bossSpawned;
        ctx.enemiesKilledInRoom = enemiesKilledInRoom;
        ctx.enemiesRequiredForBoss = enemiesRequiredForBoss;
        return ctx;
    }

    // --- SPAWN HELPERS ---

    std::vector<AEVec2> FindSafeSpawnPositions(TileMap const& tilemap, int maxCount, bool strictWallCheck = false)
    {
        std::vector<AEVec2> positions;
        auto mapSz = tilemap.GetMapSize();
        unsigned cols = mapSz.first;
        unsigned rows = mapSz.second;
        int midR = (int)rows / 2;
        int midC = (int)cols / 2;

        // Shared spacing for both passes — 3 tiles apart minimum
        const float MIN_SPACING = 115.0f * 3.0f;

        // Pass 1: TILE_ENEMY markers.
        // CSV maps (strictWallCheck=false): collect all markers as-is — designer placed them.
        // Procedural maps (strictWallCheck=true): apply 3-tile wall clearance before accepting.
        for (unsigned r = 0; r < rows; ++r) {
            for (unsigned c = 0; c < cols; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (!t || t->type != TileMap::TILE_ENEMY) continue;

                if (strictWallCheck) {
                    if (r < 2 || r >= rows - 2 || c < 2 || c >= cols - 2) continue;
                    bool nearWall = false;
                    for (int dr2 = -2; dr2 <= 2 && !nearWall; ++dr2)
                        for (int dc2 = -2; dc2 <= 2 && !nearWall; ++dc2) {
                            const TileMap::Tile* n = tilemap.QueryTile(r + dr2, c + dc2);
                            if (!n || n->isSolid) nearWall = true;
                        }
                    if (nearWall) continue;
                }

                AEVec2 candidate = tilemap.GetTilePosition(r, c);
                bool tooClose = false;
                for (AEVec2 const& existing : positions) {
                    float dx = candidate.x - existing.x;
                    float dy = candidate.y - existing.y;
                    if ((dx * dx + dy * dy) < (MIN_SPACING * MIN_SPACING)) { tooClose = true; break; }
                }
                if (!tooClose) positions.push_back(candidate);
            }
        }
        if (!positions.empty()) return positions;

        // Pass 2: procedural fallback — spaced open tiles only
        for (unsigned r = 3; r < rows - 3 && (int)positions.size() < maxCount; ++r) {
            for (unsigned c = 3; c < cols - 3 && (int)positions.size() < maxCount; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (!t || t->type != TileMap::TILE_NONE || t->isSolid) continue;

                bool nearWall = false;
                for (int dr2 = -2; dr2 <= 2 && !nearWall; ++dr2)
                    for (int dc2 = -2; dc2 <= 2 && !nearWall; ++dc2) {
                        const TileMap::Tile* n = tilemap.QueryTile(r + dr2, c + dc2);
                        if (!n || n->isSolid) nearWall = true;
                    }
                if (nearWall) continue;

                int dr = (int)r - midR, dc = (int)c - midC;
                if (dr >= -3 && dr <= 3 && dc >= -3 && dc <= 3) continue;
                if ((int)r == midR || (int)c == midC) continue;

                AEVec2 candidate = tilemap.GetTilePosition(r, c);
                bool tooClose = false;
                for (AEVec2 const& existing : positions) {
                    float dx = candidate.x - existing.x;
                    float dy = candidate.y - existing.y;
                    if ((dx * dx + dy * dy) < (MIN_SPACING * MIN_SPACING)) { tooClose = true; break; }
                }
                if (tooClose) continue;
                positions.push_back(candidate);
            }
        }
        return positions;
    }

    void SpawnProceduralEnemies(TileMap const& tilemap)
    {
        int spawnCount = 8 + rand() % 5;  // 8-12 enemies per room
        std::vector<AEVec2> spawnPositions = FindSafeSpawnPositions(tilemap, spawnCount, true);

        // Don't reset enemiesKilledInRoom or countedDeadEnemies — kills carry over between rooms.
        // Only clear the live enemy list for this room.
        procEnemies.clear();
        enemiesRequiredForBoss += (int)spawnPositions.size();
        bossMaxHPProgressBar = (float)enemiesRequiredForBoss;
        bossSpawned = false;

        for (AEVec2 const& pos : spawnPositions) {
            Enemy* e = SpawnWeightedEnemy(pos, 0.70f, 0.30f);
            if (e) {
                procEnemies.push_back(e);
                allProcEnemies.push_back(e); // track across all rooms for kill counting
            }
        }

        // Spawn a chest near the map centre as a reward for clearing the room
        AEVec2 chestPos = tilemap.GetSpawnPoint();
        chestPos.x += 150.0f;
        LootChest* chest = dynamic_cast<LootChest*>(
            GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
        if (chest) {
            chest->Init(chestPos, { 35,35 }, 0, MESH_SQUARE, Collision::COL_RECT, { 35,35 },
                CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE)
                ->GetRenderData().tint = CreateColor(255, 0.84f * 255.f, 0, 255);
        }

        std::cout << "[ProcRoom] Spawned " << procEnemies.size()
            << " enemies. Need " << enemiesRequiredForBoss << " kills for boss.\n";
    }

    void TrySpawnBoss(TileMap const& tilemap)
    {
        if (bossSpawned || !inProceduralMap) return;
        if (enemiesRequiredForBoss <= 0) return;
        float killFraction = (float)enemiesKilledInRoom / (float)enemiesRequiredForBoss;
        if (killFraction < bossSpawnThreshold) return;

        AEVec2 bossPos = tilemap.GetSpawnPoint();
        if (gPlayer) {
            bossPos = { gPlayer->GetPos().x + playerDir.x * 250.0f,
                        gPlayer->GetPos().y + playerDir.y * 250.0f };
        }
        boss = SpawnRandomBossEnemy(bossPos);
        if (!boss) return;

        bossSpawned = true;
        bossAlive = true;
        bossMaxHPProgressBar = boss->GetDefinition().baseStats.maxHP;
        bossHPProgressBar = bossMaxHPProgressBar;
        std::cout << "[Boss] Spawned " << boss->GetDefinition().name << "!\n";
    }

    // Counts proc enemy kills using a one-way ratchet — once an enemy is counted
    // as dead it is recorded in countedDeadEnemies and never counted again.
    // This prevents the count dropping when GO pool recycles a dead enemy slot.
    void CountDeadProcEnemies()
    {
        // Check ALL proc enemies ever spawned — not just the current room list
        // so kills from previous rooms still count after procEnemies.clear()
        for (Enemy* e : allProcEnemies) {
            if (!e) continue;
            if (!e->IsEnabled() || e->GetHP() <= 0.f) {
                bool alreadyCounted = false;
                for (Enemy* counted : countedDeadEnemies) {
                    if (counted == e) { alreadyCounted = true; break; }
                }
                if (!alreadyCounted) {
                    countedDeadEnemies.push_back(e);
                    ++enemiesKilledInRoom;
                }
            }
        }
    }

    // --- CAMERA ---
    void UpdateWorldMap(float dt) {
        float winW = (float)AEGfxGetWinMaxX(), winH = (float)AEGfxGetWinMaxY();
        float viewHalfW = (winW * 0.5f) / camZoom, viewHalfH = (winH * 0.5f) / camZoom;

        AEVec2 camTarget = GetPlayerPos();
        float limitX = halfMapWidth - viewHalfW, limitY = halfMapHeight - viewHalfH;
        if (limitX > 0) camTarget.x = AEClamp(camTarget.x, -limitX, limitX); else camTarget.x = 0;
        if (limitY > 0) camTarget.y = AEClamp(camTarget.y, -limitY, limitY); else camTarget.y = 0;

        float omega = 2.0f / camSmoothTime, xd = omega * dt;
        float expK = 1.0f / (1.0f + xd + 0.48f * xd * xd + 0.235f * xd * xd * xd);
        AEVec2 change = { camPos.x - camTarget.x, camPos.y - camTarget.y };
        AEVec2 temp = { (camVel.x + omega * change.x) * dt, (camVel.y + omega * change.y) * dt };
        camVel.x = (camVel.x - omega * temp.x) * expK;
        camVel.y = (camVel.y - omega * temp.y) * expK;
        camPos.x = camTarget.x + (change.x + temp.x) * expK;
        camPos.y = camTarget.y + (change.y + temp.y) * expK;
        SetCameraPos(camPos); SetCamZoom(camZoom);
    }

    void RenderWorldMap() {
        if (!bossAlive) {
            DrawTintedMesh(GetTransformMtx({ currentLevel.doorPos.x - 335.0f, currentLevel.doorPos.y }, 0, { 45,125 }),
                wallMesh, nullptr, { 0, 0.8f * 255, 0, 255 }, 255);
        }
        if (inProceduralMap) { if (nextMap) nextMap->Render(); }
        else { map->Render(); }
    }

    // GREEN  = kill progress (before boss spawns)
    // RED    = boss HP bar (after boss spawns)
    // GREY   = boss dead
    void DrawBossHPProgressBar()
    {
        if (doTutorial && fairy->data.stage != Tutorial::BOSS) return;

        bossHPProgressBarHeight = 50.f;
        bossHPProgressBarWidth = (float)AEGfxGetWinMaxX() - (float)AEGfxGetWinMinX();
        float barX = -bossHPProgressBarWidth * 0.5f;
        float barY = (float)AEGfxGetWinMaxY() - bossHPProgressBarHeight * 0.5f;
        float bhpbMargin = 4.f;

        float killFraction = (enemiesRequiredForBoss > 0)
            ? AEClamp((float)enemiesKilledInRoom / (float)enemiesRequiredForBoss, 0.f, 1.f)
            : 0.f;
        float hpRatio = (bossSpawned && bossMaxHPProgressBar > 0)
            ? AEClamp(bossHPProgressBar / bossMaxHPProgressBar, 0.f, 1.f)
            : killFraction;
        float fillRatio = bossSpawned ? hpRatio : killFraction;

        AEVec2 bgSize = ToVec2(bossHPProgressBarWidth, bossHPProgressBarHeight);
        AEVec2 bhpbSize = ToVec2((bossHPProgressBarWidth - bhpbMargin) * fillRatio, bossHPProgressBarHeight - bhpbMargin);
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
        if (!bossSpawned)   AEGfxSetColorToMultiply(0.2f, 0.7f, 0.2f, 1.f);
        else if (bossAlive) AEGfxSetColorToMultiply(0.7f, 0.2f, 0.2f, 1.f);
        else                AEGfxSetColorToMultiply(0.9f, 0.9f, 0.9f, 1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }
}

// =============================================================
void GameState::LoadState() {
    // =============================================================
    if (!GameDB::LoadEnemyDefs("Assets/Data/enemies.json"))
        std::cout << "WARNING: enemies.json failed to load.\n";

    font = RenderingManager::GetInstance()->GetFont();

    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Melee/melee.json", EquipmentCategory::Melee);
    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Range/ranged.json", EquipmentCategory::Ranged);
    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Armor/Head/head.json", EquipmentCategory::Head);
    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Armor/Body/body.json", EquipmentCategory::Body);
    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Armor/Hands/hands.json", EquipmentCategory::Hands);
    GameDB::LoadEquipmentDefs("Assets/Data/Equipment/Armor/Feet/feet.json", EquipmentCategory::Feet);

    if (!GameDB::LoadDropTables("Assets/Data/Drops/drops.json"))
        std::cout << "WARNING: drops.json failed to load.\n";
    if (!GameDB::LoadPlayerDef("Assets/Data/Player/player.json"))
        std::cout << "WARNING: player.json failed to load.\n";
    if (!GameDB::LoadPlayerInventory("Assets/Data/Player/inventory.json"))
        std::cout << "WARNING: inventory.json failed to load.\n";

    bgm.Init();
    halfMapWidth = mapWidth * 0.5f;
    halfMapHeight = mapHeight * 0.5f;
    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);
    squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

    map = new TileMap("Assets/Dungeon.csv", { 0,0 }, 115.f, 115.f);

    float    procTileSize = 115.f;
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
    mapHeight = (map->GetFullMapSize().y > nextMap->GetFullMapSize().y)
        ? map->GetFullMapSize().y : nextMap->GetFullMapSize().y;

    GameObjectManager::GetInstance()->InitCollisionGrid(
        static_cast<unsigned>(mapWidth),
        static_cast<unsigned>(mapHeight)
    );

    if (!gPlayer) gPlayer = new Player();
    PetManager::GetInstance()->LinkPlayer(gPlayer);

    boss = nullptr;

    if (doTutorial)
        fairy = new Tutorial::TutorialFairy();
}

// =============================================================
void GameState::InitState()
{
    // =============================================================
    bgm.PlayNormal();
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

    AEVec2 chestTilePos = currentLevel.chestPos;
    bool   foundChest = false;
    for (unsigned r = 0; r < csvRows; ++r) {
        for (unsigned c = 0; c < csvCols; ++c) {
            const TileMap::Tile* t = map->QueryTile(r, c);
            if (!t) continue;
            if (t->type == TileMap::TILE_CHEST && !foundChest) {
                chestTilePos = map->GetTilePosition(r, c);
                foundChest = true;
            }
        }
    }

    Bitmask collideMask = CreateBitmask(3,
        Collision::LAYER::ENEMIES,
        Collision::LAYER::INTERACTABLE,
        Collision::LAYER::OBSTACLE
    );

    gPlayer->Init(
        safeSpawnPos,
        AEVec2{ playerRadius * 2.f, playerRadius * 2.f },
        0, MESH_CIRCLE, Collision::SHAPE::COL_CIRCLE,
        AEVec2{ playerRadius * 2.f, playerRadius * 2.f },
        collideMask, Collision::LAYER::PLAYER
    );
    PetManager::GetInstance()->InitPetForGame(*map);

    ActorStats base{};
    base.maxHP = 100.0f;
    base.attack = 10.0f;
    base.attackSpeed = 1.0f;
    base.moveSpeed = playerSpeed;
    gPlayer->InitPlayerRuntime(base);
    gPlayer->ApplyShopUpgrades();
    gPlayer->Heal(gPlayer->GetMaxHP());

    // --- CSV chest (unchanged) ---
    LootChest* chest = dynamic_cast<LootChest*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
    chest->Init(chestTilePos, { 35,35 }, 0, MESH_SQUARE, Collision::COL_RECT, { 35,35 },
        CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE)
        ->GetRenderData().tint = CreateColor(255, 0.84f * 255.f, 0, 255);

    // --- CSV enemies (unchanged) ---
    std::vector<AEVec2> csvSpawns = FindSafeSpawnPositions(*map, 0);
    csvEnemies.clear();
    for (AEVec2 const& pos : csvSpawns) {
        Enemy* e = SpawnWeightedEnemy(pos, 0.70f, 0.30f);
        if (e) {
            const char* tier = (e->GetDefinition().category == EnemyCategory::Elite) ? "Elite" : "Normal";
            std::cout << "[CSV] " << e->GetDefinition().name << " (" << tier << ")\n";
            csvEnemies.push_back(e);
        }
    }

    boss = nullptr;
    bossSpawned = false;
    bossAlive = true;
    enemiesKilledInRoom = 0;
    enemiesRequiredForBoss = (int)csvEnemies.size();
    bossMaxHPProgressBar = (float)enemiesRequiredForBoss;
    bossHPProgressBar = 0.f;
    countedDeadEnemies.clear();
    allProcEnemies.clear();

    camPos = safeSpawnPos;
    camVel = { 0, 0 };
    minimap->Reset();

    if (doTutorial)
        fairy->InitTutorial(gPlayer, &currentLevel);
}

// =============================================================
void GameState::Update(double dt)
{
    // =============================================================
    if (AEInputCheckTriggered(AEVK_M)) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
        return;
    }

    if (AEInputCheckTriggered(AEVK_TAB)) {
        debugMode = !debugMode;
        showDebugOverlay = debugMode;
        std::cout << "[Debug] Mode " << (debugMode ? "ON" : "OFF") << "\n";
    }

    if (AEInputCheckTriggered(AEVK_K)) {
        showKeybindOverlay = !showKeybindOverlay;
    }

    if (debugMode) {
        if (AEInputCheckTriggered(AEVK_F5)) { debugGodMode = !debugGodMode;       std::cout << "[Debug] God mode " << (debugGodMode ? "ON" : "OFF") << "\n"; }
        if (AEInputCheckTriggered(AEVK_F6)) { debugShowStats = !debugShowStats;     std::cout << "[Debug] Live stats " << (debugShowStats ? "ON" : "OFF") << "\n"; }
        if (AEInputCheckTriggered(AEVK_F7)) { debugFreezeEnemies = !debugFreezeEnemies; std::cout << "[Debug] Freeze AI " << (debugFreezeEnemies ? "ON" : "OFF") << "\n"; }

        if (AEInputCheckTriggered(AEVK_F1)) {
            for (Enemy* e : procEnemies) if (e && e->IsEnabled()) e->TakeDamage({ 99999.f, nullptr, DAMAGE_TYPE::TRUE_DAMAGE, nullptr });
            for (Enemy* e : csvEnemies)  if (e && e->IsEnabled()) e->TakeDamage({ 99999.f, nullptr, DAMAGE_TYPE::TRUE_DAMAGE, nullptr });
            std::cout << "[Debug] All enemies killed.\n";
        }
        if (AEInputCheckTriggered(AEVK_F2)) {
            if (!bossSpawned && inProceduralMap && nextMap) {
                enemiesKilledInRoom = enemiesRequiredForBoss;
                TrySpawnBoss(*nextMap);
                std::cout << "[Debug] Boss force-spawned.\n";
            }
        }
        if (AEInputCheckTriggered(AEVK_F3)) {
            if (gPlayer) {
                TileMap* cur = inProceduralMap ? nextMap : map;
                AEVec2 sp = cur ? cur->GetSpawnPoint() : AEVec2{ 0,0 };
                gPlayer->SetPos(sp);
                camPos = sp;
                std::cout << "[Debug] Teleported to spawn.\n";
            }
        }
        if (AEInputCheckTriggered(AEVK_F4)) {
            if (gPlayer) { gPlayer->Heal(gPlayer->GetMaxHP()); std::cout << "[Debug] HP refilled.\n"; }
        }
    }

#pragma region inputs_for_testing
    if (AEInputCheckTriggered(AEVK_L)) {
        LootChest* chest = dynamic_cast<LootChest*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
        AEVec2 m = GetMouseWorldVec();
        chest->Init(m, { 75,75 }, 1, MESH_SQUARE, Collision::COL_RECT, { 75,75 },
            CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE);
    }
    if (AEInputCheckTriggered(AEVK_R)) {
        PostOffice::GetInstance()->Send("PetManager", new PetSkillMsg(PetSkillMsg::CAST_SKILL));
    }
    if (AEInputCheckTriggered(AEVK_N)) {
        AEVec2 m = GetMouseWorldVec();
        Enemy* e = SpawnWeightedEnemy(m, 0.70f, 0.30f);
        if (e) {
            const char* tier = (e->GetDefinition().category == EnemyCategory::Elite) ? "Elite" : "Normal";
            std::cout << "[Spawn N] " << e->GetDefinition().name << " (" << tier << ")\n";
            if (inProceduralMap) procEnemies.push_back(e);
            else                 csvEnemies.push_back(e);
            ++enemiesRequiredForBoss;
            bossMaxHPProgressBar = (float)enemiesRequiredForBoss;
        }
    }
#pragma endregion

    if (!gPlayer) return;

    if (debugMode && debugGodMode && gPlayer)
        gPlayer->Heal(gPlayer->GetMaxHP());

    if (teleportCooldown > 0.f) teleportCooldown -= (float)dt;

    if (teleportCooldown <= 0.f && nextMap) {
        if (!inProceduralMap && map->IsConnector(gPlayer->GetPos())) {
            nextMap->GenerateProcedural(50, 50, rand());
            AEVec2 procSpawn = nextMap->GetSpawnPoint();
            gPlayer->SetPos(procSpawn);
            gPlayer->Move({ 0,0 });
            PetManager::GetInstance()->PlacePet(gPlayer->GetPos());
            camPos = procSpawn; camVel = { 0, 0 };
            halfMapWidth = nextMap->GetFullMapSize().x * 0.5f;
            halfMapHeight = nextMap->GetFullMapSize().y * 0.5f;
            SetCameraPos(camPos);
            inProceduralMap = true;
            teleportCooldown = 2.f;
            // Fresh start for procedural kill tracking
            enemiesKilledInRoom = 0;
            enemiesRequiredForBoss = 0;
            bossMaxHPProgressBar = 1.f;
            countedDeadEnemies.clear();
            allProcEnemies.clear();
            SpawnProceduralEnemies(*nextMap);
            minimap->Reset();
            PetManager::GetInstance()->SetTilemap(*nextMap);
        }
        else if (inProceduralMap && nextMap->IsConnector(gPlayer->GetPos())) {
            nextMap->GenerateProcedural(50, 50, rand());
            AEVec2 procSpawn = nextMap->GetSpawnPoint();
            gPlayer->SetPos(procSpawn);
            gPlayer->Move({ 0,0 });
            PetManager::GetInstance()->PlacePet(gPlayer->GetPos());
            camPos = procSpawn; camVel = { 0, 0 };
            halfMapWidth = nextMap->GetFullMapSize().x * 0.5f;
            halfMapHeight = nextMap->GetFullMapSize().y * 0.5f;
            SetCameraPos(camPos);
            teleportCooldown = 2.f;
            // Keep enemiesKilledInRoom and countedDeadEnemies — kills carry over
            SpawnProceduralEnemies(*nextMap);
            minimap->Reset();
        }
    }

    TileMap* currentMap = inProceduralMap ? nextMap : map;

    AEVec2 move = gPlayer->GetMoveDirNorm();
    f32 len = AEVec2Length(&move);
    if (len > 0 || gPlayer->HasForceApplied())
        playerDir = gPlayer->GetMoveDirNorm();

    if (inProceduralMap) {
        if (!(debugMode && debugFreezeEnemies)) CountDeadProcEnemies();
        TrySpawnBoss(*nextMap);
    }
    else {
        // CSV kill counting — unchanged
        if (!(debugMode && debugFreezeEnemies)) {
            int dead = 0;
            for (Enemy* e : csvEnemies)
                if (e && (!e->IsEnabled() || e->GetHP() <= 0.f)) ++dead;
            enemiesKilledInRoom = dead;
        }
    }

    if (boss && bossSpawned) {
        bossHPProgressBar = (boss->GetHP() / boss->GetMaxHP()) * bossMaxHPProgressBar;
        bossAlive = !boss->IsDead();
    }

    if (doTutorial && fairy->data.stage == Tutorial::BOSS && !bossAlive)
        fairy->ChangeStage(Tutorial::END);

    if (!doTutorial && bossSpawned && !bossAlive) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
        std::cout << "BOSS SLAYED\n";
    }

    minimap->Update(dt, *currentMap, *gPlayer);
    UpdateWorldMap((float)dt);

    if (debugMode && debugFreezeEnemies) {
        auto& gameObjects = GameObjectManager::GetInstance()->GetGameObjects();
        std::vector<std::pair<GameObject*, AEVec2>> frozenEnemies;
        for (int i = 0; i < (int)gameObjects.size(); ++i) {
            GameObject* obj = gameObjects[i];
            if (obj && obj->IsEnabled() && obj->GetGOType() == GO_TYPE::ENEMY)
                frozenEnemies.push_back(std::make_pair(obj, obj->GetPos()));
        }
        GameObjectManager::GetInstance()->UpdateObjects(dt, currentMap);
        for (int i = 0; i < (int)frozenEnemies.size(); ++i) {
            GameObject* obj = frozenEnemies[i].first;
            AEVec2      savedPos = frozenEnemies[i].second;
            if (!obj->IsEnabled()) continue;
            Enemy* enemy = dynamic_cast<Enemy*>(obj);
            if (enemy && enemy->GetHP() > 0.f) enemy->SetPos(savedPos);
        }
    }
    else {
        GameObjectManager::GetInstance()->UpdateObjects(dt, currentMap);
    }

    DropSystem::PrintPickupDisplay(static_cast<float>(dt));
}

// =============================================================
void GameState::Draw() {
    // =============================================================
    RenderWorldMap();
    GameObjectManager::GetInstance()->DrawObjects();
    DrawBossHPProgressBar();

    TileMap* currentMap = inProceduralMap ? nextMap : map;
    minimap->Render(*currentMap, *gPlayer);

    if (debugMode) {
        DebugContext dbg = MakeDebugCtx();
        if (debugShowStats)   DrawEnemyStats(dbg);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        if (showDebugOverlay) DrawDebugOverlay(dbg);
    }

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    if (showKeybindOverlay) DrawKeybindOverlay(MakeDebugCtx());

    if (gPlayer) gPlayer->DrawUI();
    HandleTutorialDialogueRender();
    PetManager::GetInstance()->DrawUI();
}

void GameState::HandleTutorialDialogueRender()
{
    if (!doTutorial || !fairy || !fairy->data.playDialogue) return;
    DrawAEText(font, fairy->data.dialogueLines[fairy->data.currDialogueLine].c_str(),
        fairy->data.dialoguePos, fairy->data.dialogueSize,
        CreateColor(238, 128, 238, 255), TEXT_MIDDLE, 1);
}

void GameState::ExitState() {
    bgm.StopGameplayBGM();
    gPlayer->ClearStatusEffects();
    inProceduralMap = false;
    teleportCooldown = 0.f;
    bossSpawned = false;
    bossAlive = true;
    enemiesKilledInRoom = 0;
    enemiesRequiredForBoss = 0;
    countedDeadEnemies.clear();
    allProcEnemies.clear();
    debugMode = false;
    showDebugOverlay = false;
    showKeybindOverlay = false;
    debugGodMode = false;
    debugShowStats = false;
    debugFreezeEnemies = false;
}

void GameState::UnloadState() {
    if (wallMesh) AEGfxMeshFree(wallMesh);

    delete map;     map = nullptr;
    delete nextMap; nextMap = nullptr;
    delete minimap; minimap = nullptr;

    gPlayer = nullptr;
    boss = nullptr;
    csvEnemies.clear();
    procEnemies.clear();
    allProcEnemies.clear();
    countedDeadEnemies.clear();

    if (doTutorial && fairy) { delete fairy; fairy = nullptr; }

    bgm.Exit();
    if (font >= 0) { AEGfxDestroyFont(font); font = -1; }
}

bool getBossAlive() { return bossAlive; }
float getBossHPProgressBar() { return bossHPProgressBar; }
void setBossHPProgressBar(float current) { bossHPProgressBar = current; }
float getBossMaxHPProgressBar() { return bossMaxHPProgressBar; }
void setBossMaxHPProgressBar(float max) { bossMaxHPProgressBar = max; }