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
#include "../GameStates/LevelSelectState.h"
#include "../ShopFunctions.h"
#include "../Pause.h"

namespace {
    // --- GLOBAL SYSTEMS ---
    AEGfxVertexList* circleMesh = nullptr;
    AEGfxVertexList* wallMesh = nullptr;
    AEGfxVertexList* squareMesh = nullptr;

    // --- PLAYER DATA ---
    AEVec2  playerPos;
    AEVec2  playerDir = { 1.0f, 0.0f };
    Player* gPlayer = nullptr;
    //float   playerRadius = 15.0f;
    float   playerSpeed = 300.0f;

    static AEVec2 GetPlayerPos()
    {
        return gPlayer ? gPlayer->GetPos() : AEVec2{ 0.0f, 0.0f };
    }

    // --- BOSS / ENEMY TRACKING ---
    bool   bossAlive = true;
    bool   bossSpawned = false;
    float  bossRadius = 60.0f;
    Enemy* boss = nullptr;

    std::vector<Enemy*> csvEnemies;
    std::vector<Enemy*> procEnemies;

    int   enemiesKilledInRoom = 0;
    int   enemiesRequiredForBoss = 0;

    // -------------------------------------------------------
    // PROGRESS BAR
    // totalKillTarget      : random 20-50, set once per run.
    // totalEnemiesKilled   : running kill count, carries forward.
    // previousRoomsKilled  : snapshot before list clears on transition.
    // -------------------------------------------------------
    int   totalEnemiesKilled = 0;
    int   totalEnemiesRequired = 0;
    int   previousRoomsKilled = 0;
    int   totalKillTarget = 0;

    float bossSpawnThreshold = 1.0f;

    // -------------------------------------------------------
    // PROCEDURAL WAVE SPAWNING
    // Wave 0 fires immediately on room entry with 8 enemies.
    // Each wave after that fires every PROC_WAVE_INTERVAL seconds
    // and grows by 1: wave 1=2, wave 2=3, wave 3=4, ...
    // -------------------------------------------------------
    float procWaveTimer = 0.0f;
    int   procWaveNumber = 0;
    const float PROC_WAVE_INTERVAL = 8.0f;

    // --- CAMERA ---
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
    bool doTutorial{ true };
    Tutorial::TutorialFairy* fairy;
    s8 font{ -1 };

    // --- BOSS HP / PROGRESS BAR ---
    float bossHPProgressBarWidth = 0.f;
    float bossHPProgressBarHeight = 0.f;
    float bossHPProgressBar = 100.f;
    float bossMaxHPProgressBar = 100.f;

    // --- DEBUG ---
    bool debugMode = false;
    bool showDebugOverlay = false;
    bool showKeybindOverlay = false;
    bool debugGodMode = false;
    bool debugFreezeEnemies = false;
    bool debugShowChests = false;   // F7 — highlight all chest positions

    // --- LOADING SCREEN ---
    // > 0 while the loading screen is visible.  Counts down in seconds.
    float loadingTimer = 0.f;
    const float LOADING_DURATION = 7.f;

    // --- ENDLESS SURVIVAL TIMER ---
    float endlessRunTimer = 0.f;
    bool endlessTimerActive = false;

    DebugContext MakeDebugCtx()
    {
        DebugContext ctx;
        ctx.squareMesh = squareMesh;
        ctx.font = font;
        ctx.gPlayer = gPlayer;
        ctx.showDebugOverlay = showDebugOverlay;
        ctx.showKeybindOverlay = showKeybindOverlay;
        ctx.debugGodMode = debugGodMode;
        ctx.debugFreezeEnemies = debugFreezeEnemies;
        ctx.debugShowChests = debugShowChests;
        ctx.inProceduralMap = inProceduralMap;
        ctx.bossSpawned = bossSpawned;
        ctx.enemiesKilledInRoom = enemiesKilledInRoom;
        ctx.enemiesRequiredForBoss = enemiesRequiredForBoss;
        ctx.totalEnemiesKilled = totalEnemiesKilled;
        ctx.totalKillTarget = totalKillTarget;
        ctx.csvEnemyCount = (int)csvEnemies.size();
        ctx.procEnemyCount = (int)procEnemies.size();
        return ctx;
    }

    // =========================================================
    // SPAWN HELPERS
    // =========================================================

    // Finds safe spawn positions from a tilemap.
    // For CSV maps (trustMarkers=true) designer-placed TILE_ENEMY markers
    // are trusted as-is. For proc maps markers must pass clearance check.
    std::vector<AEVec2> FindSafeSpawnPositions(TileMap const& tilemap, int maxCount, bool trustMarkers = true)
    {
        std::vector<AEVec2> positions;
        auto mapSz = tilemap.GetMapSize();
        unsigned cols = mapSz.first;
        unsigned rows = mapSz.second;

        // Pass 1: TILE_ENEMY markers
        for (unsigned r = 0; r < rows; ++r) {
            for (unsigned c = 0; c < cols; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (!t || t->type != TileMap::TILE_ENEMY) continue;
                if (!trustMarkers && !tilemap.HasClearance(r, c, 2)) continue;
                positions.push_back(tilemap.GetTilePosition(r, c));
                std::cout << "[FindSafe] TILE_ENEMY at (" << r << "," << c << ") accepted.\n";
            }
        }
        if (!positions.empty()) return positions;

        std::cout << "[FindSafe] No TILE_ENEMY markers — falling back to open tile scan.\n";

        // Pass 2: scan open tiles with clearance as fallback
        int midR = (int)rows / 2;
        int midC = (int)cols / 2;
        const float MIN_SPACING = 115.0f * 3.0f;

        for (unsigned r = 3; r < rows - 3 && (int)positions.size() < maxCount; ++r) {
            for (unsigned c = 3; c < cols - 3 && (int)positions.size() < maxCount; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (!t || t->type != TileMap::TILE_NONE || t->isSolid) continue;
                if (!tilemap.HasClearance(r, c, 2)) continue;

                int dr = (int)r - midR, dc = (int)c - midC;
                if (dr >= -3 && dr <= 3 && dc >= -3 && dc <= 3) continue;
                if ((int)r == midR || (int)c == midC) continue;

                AEVec2 candidate = tilemap.GetTilePosition(r, c);
                bool tooClose = false;
                for (AEVec2 const& existing : positions) {
                    float dx = candidate.x - existing.x;
                    float dy = candidate.y - existing.y;
                    if ((dx * dx + dy * dy) < (MIN_SPACING * MIN_SPACING)) {
                        tooClose = true; break;
                    }
                }
                if (tooClose) continue;
                positions.push_back(candidate);
            }
        }
        return positions;
    }

    // Collects wall-free open tiles from the proc map.
    // Uses HasClearance(2) and avoids the player spawn centre and corridors.
    std::vector<AEVec2> CollectProcSafePositions(TileMap const& tilemap)
    {
        std::vector<AEVec2> positions;
        auto mapSz = tilemap.GetMapSize();
        unsigned cols = mapSz.first;
        unsigned rows = mapSz.second;
        int midR = (int)rows / 2;
        int midC = (int)cols / 2;

        for (unsigned r = 5; r < rows - 5; ++r) {
            for (unsigned c = 5; c < cols - 5; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (!t || t->type != TileMap::TILE_NONE || t->isSolid) continue;
                if (!tilemap.HasClearance(r, c, 2)) continue;

                int dr = (int)r - midR, dc = (int)c - midC;
                if (dr >= -5 && dr <= 5 && dc >= -5 && dc <= 5) continue;
                if ((int)r == midR || (int)c == midC) continue;

                positions.push_back(tilemap.GetTilePosition(r, c));
            }
        }
        std::cout << "[CollectProc] Found " << positions.size() << " safe positions.\n";
        return positions;
    }

    // Spawns LootChest objects at every TILE_CHEST marker in a CSV tilemap.
    // Only used for CSV maps — proc maps use SpawnProcChests instead.
    void SpawnCsvChests(TileMap const& tilemap)
    {
        auto mapSz = tilemap.GetMapSize();
        unsigned cols = mapSz.first;
        unsigned rows = mapSz.second;

        for (unsigned r = 0; r < rows; ++r) {
            for (unsigned c = 0; c < cols; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (!t || t->type != TileMap::TILE_CHEST) continue;

                AEVec2 pos = tilemap.GetTilePosition(r, c);
                LootChest* chest = dynamic_cast<LootChest*>(
                    GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
                if (!chest) continue;

                chest->Init(pos, { 35,35 }, 0, MESH_SQUARE, Collision::COL_RECT, { 35,35 },
                    CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE);
                std::cout << "[SpawnCsvChests] Chest at (" << pos.x << ", " << pos.y << ")\n";

                if (doTutorial) {
                    chest->SetDropTable(2);
                }
            }
        }
    }

    // Spawns 1-3 chests at random safe positions in a proc map.
    // Positions are picked the same way as enemy waves so chests
    // never appear inside walls.
    void SpawnProcChests(TileMap const& tilemap)
    {
        std::vector<AEVec2> safePool = CollectProcSafePositions(tilemap);
        if (safePool.empty()) return;

        int chestCount = 3 + rand() % 3;
        int spawned = 0;

        for (int i = 0; i < chestCount && !safePool.empty(); ++i) {
            int    idx = rand() % (int)safePool.size();
            AEVec2 pos = safePool[idx];
            safePool.erase(safePool.begin() + idx);

            LootChest* chest = dynamic_cast<LootChest*>(
                GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
            if (!chest) continue;

            chest->Init(pos, { 35,35 }, 0, MESH_SQUARE, Collision::COL_RECT, { 35,35 },
                CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE);
            ++spawned;
        }
        std::cout << "[SpawnProcChests] Spawned " << spawned << " chests.\n";
    }

    // Disables every enemy in a list and clears it so they vanish from
    // the world cleanly without bleeding into the next map.
    void DisableAndClearEnemies(std::vector<Enemy*>& enemies)
    {
        for (Enemy* e : enemies)
            if (e && e->IsEnabled()) e->SetEnabled(false);
        enemies.clear();
    }

    // Wave 0 = 8 enemies, fired immediately when entering a proc room.
    // Wave 1 = 2, wave 2 = 3, wave 3 = 4, ... growing by 1 each time.
    // All positions come from CollectProcSafePositions so no wall spawns.
    // Min spacing of 3 tiles between picks prevents enemies from overlapping.
    // Hard cap of MAX_LIVE_ENEMIES
    void SpawnProcWave(TileMap const& tilemap)
    {
        // Hard cap: ever let more than this many enemies be active at once
        const int MAX_LIVE_ENEMIES = 30;
        // Minimum world-space gap between two spawn positions (~3 tiles at 32 px)
        const float MIN_SPAWN_SPACING = 32.f * 3.f;

        // Count currently live enemies before spawning
        int liveCount = 0;
        for (Enemy* e : procEnemies)
            if (e && e->IsEnabled() && e->GetHP() > 0.f) ++liveCount;

        int canSpawn = MAX_LIVE_ENEMIES - liveCount;
        if (canSpawn <= 0) {
            std::cout << "[ProcWave] Live enemy cap hit (" << liveCount << "), skipping wave.\n";
            ++procWaveNumber; // still advance so timer doesn't keep firing immediately
            return;
        }

        int waveSize = (procWaveNumber == 0) ? 8 : (1 + procWaveNumber);
        waveSize = (waveSize < canSpawn) ? waveSize : canSpawn; // don't exceed cap
        std::cout << "[ProcWave] procWaveNumber=" << procWaveNumber
            << " waveSize=" << waveSize << " (live=" << liveCount << ")\n";
        ++procWaveNumber;

        std::vector<AEVec2> safePool = CollectProcSafePositions(tilemap);
        if (safePool.empty()) {
            std::cout << "[ProcWave] No safe positions found!\n";
            return;
        }

        // Track already-chosen positions and enforce min spacing between them
        std::vector<AEVec2> chosen;
        int spawned = 0;

        for (int i = 0; i < waveSize && !safePool.empty(); ++i) {
            // Try up to 20 random picks looking for a well-spaced slot
            bool placed = false;
            for (int attempt = 0; attempt < 20 && !safePool.empty(); ++attempt) {
                int    idx = rand() % (int)safePool.size();
                AEVec2 pos = safePool[idx];

                // Reject if too close to an already-chosen position this wave
                bool tooClose = false;
                for (AEVec2 const& c : chosen) {
                    float dx = pos.x - c.x, dy = pos.y - c.y;
                    if ((dx * dx + dy * dy) < MIN_SPAWN_SPACING * MIN_SPAWN_SPACING) {
                        tooClose = true;
                        break;
                    }
                }

                safePool.erase(safePool.begin() + idx);
                if (tooClose) continue;

                Enemy* e = SpawnWeightedEnemy(pos, 0.70f, 0.30f);
                if (!e) continue;
                procEnemies.push_back(e);
                chosen.push_back(pos);
                ++spawned;
                placed = true;
                break;
            }
            if (!placed) break; // pool exhausted or no spaced slot found
        }
        std::cout << "[ProcWave " << procWaveNumber << "] Spawned "
            << spawned << " enemies (wave size " << waveSize << ").\n";
    }

    // Checks if enough enemies have been killed to spawn the boss.
    // Clears all remaining regular enemies when boss appears.
    void TrySpawnBoss(TileMap const& tilemap)
    {
        if (bossSpawned || !inProceduralMap) return;
        if (totalKillTarget <= 0) return;
        float killFraction = (float)totalEnemiesKilled / (float)totalKillTarget;
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

        // Clear regular enemies so player only fights the boss
        DisableAndClearEnemies(procEnemies);
        DisableAndClearEnemies(csvEnemies);

        std::cout << "[Boss] Spawned " << boss->GetDefinition().name
            << "! (kill target was " << totalKillTarget << ")\n";
    }

    // Counts dead enemies from both lists every frame.
    // previousRoomsKilled preserves kills from cleared rooms so the
    // bar never resets when lists are wiped on room transition.
    void CountAllDeadEnemies()
    {
        int dead = 0;
        for (Enemy* e : csvEnemies)
            if (e && (!e->IsEnabled() || e->GetHP() <= 0.f)) ++dead;
        for (Enemy* e : procEnemies)
            if (e && (!e->IsEnabled() || e->GetHP() <= 0.f)) ++dead;

        totalEnemiesKilled = previousRoomsKilled + dead;
        enemiesKilledInRoom = dead;
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
        AEVec2 temp = { (camVel.x + omega * change.x) * dt,
                          (camVel.y + omega * change.y) * dt };
        camVel.x = (camVel.x - omega * temp.x) * expK;
        camVel.y = (camVel.y - omega * temp.y) * expK;
        camPos.x = camTarget.x + (change.x + temp.x) * expK;
        camPos.y = camTarget.y + (change.y + temp.y) * expK;
        SetCameraPos(camPos);
        SetCamZoom(camZoom);
    }

    void RenderWorldMap() {
        // Draw green door marker when boss is defeated
        if (!bossAlive) {
            DrawTintedMesh(
                GetTransformMtx({ currentLevel.doorPos.x, currentLevel.doorPos.y },
                    0, { 80, 80 }),
                wallMesh, nullptr, { 0, 0.8f * 255, 0, 255 }, 255);
        }
        if (inProceduralMap) { if (nextMap) nextMap->Render(); }
        else { map->Render(); }
    }

    // Progress bar colours:
    // Green  = kill progress before boss spawns
    // Red    = boss HP after boss spawns
    // Grey   = boss is dead
    void DrawBossHPProgressBar()
    {
        if (doTutorial && fairy->data.stage != Tutorial::BOSS) return;

        bossHPProgressBarHeight = 50.f;
        bossHPProgressBarWidth = (float)AEGfxGetWinMaxX() - (float)AEGfxGetWinMinX();
        float barX = -bossHPProgressBarWidth * 0.5f;
        float barY = (float)AEGfxGetWinMaxY() - bossHPProgressBarHeight * 0.5f;
        float bhpbMargin = 4.f;

        float killFraction = (totalKillTarget > 0)
            ? AEClamp((float)totalEnemiesKilled / (float)totalKillTarget, 0.f, 1.f)
            : 0.f;
        float hpRatio = (bossSpawned && bossMaxHPProgressBar > 0)
            ? AEClamp(bossHPProgressBar / bossMaxHPProgressBar, 0.f, 1.f)
            : killFraction;
        float fillRatio = bossSpawned ? hpRatio : killFraction;

        AEVec2 bgSize = ToVec2(bossHPProgressBarWidth, bossHPProgressBarHeight);
        AEVec2 bhpbSize = ToVec2((bossHPProgressBarWidth - bhpbMargin) * fillRatio,
            bossHPProgressBarHeight - bhpbMargin);
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
void GameState::LoadState()
{
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

    // Endless has no CSV map so create an empty tilemap as placeholder
    if (mapSelected == "Assets/Endless.csv") {
        map = new TileMap({ 0.f, 0.f }, 115.f, 115.f);
        std::cout << "[LoadState] Endless mode — no CSV map.\n";
    }
    else {
        map = new TileMap(mapSelected, { 0,0 }, 115.f, 115.f);
        std::cout << "[LoadState] Loaded map: " << mapSelected << "\n";
    }

    if (mapSelected == "Assets/TutorialMap.csv") {
        doTutorial = true;
        fairy = new Tutorial::TutorialFairy();
    }

    float    procTileSize = 115.f;
    unsigned procRows = 50, procCols = 50;
    nextMap = new TileMap({ 0.f, 0.f }, procTileSize, procTileSize);
    srand(1234);
    nextMap->GenerateProcedural(procRows, procCols, 1234);

    // Find a safe spawn tile in the CSV map for Tutorial and Normal modes
    if (mapSelected != "Assets/Endless.csv") {
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
    }

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
}

// =============================================================
void GameState::InitState()
{
    doTutorial = (mapSelected == "Assets/TutorialMap.csv");
    bgm.PlayNormal();
    InitTutorial(currentLevel);

    // Reload the correct CSV map in case the player changed mode and came back
    if (map) { delete map; map = nullptr; }
    if (mapSelected == "Assets/Endless.csv") {
        map = new TileMap({ 0.f, 0.f }, 115.f, 115.f);
    }
    else {
        map = new TileMap(mapSelected, { 0,0 }, 115.f, 115.f);
        std::cout << "[InitState] Reloaded map: " << mapSelected << "\n";
    }

    // Recompute map bounds after reload
    mapWidth = map->GetFullMapSize().x + nextMap->GetFullMapSize().x;
    mapHeight = (map->GetFullMapSize().y > nextMap->GetFullMapSize().y)
        ? map->GetFullMapSize().y : nextMap->GetFullMapSize().y;
    halfMapWidth = mapWidth * 0.5f;
    halfMapHeight = mapHeight * 0.5f;

    // Reset all counters before anything spawns
    boss = nullptr;
    bossSpawned = false;
    bossAlive = true;
    enemiesKilledInRoom = 0;
    enemiesRequiredForBoss = 0;
    totalEnemiesKilled = 0;
    totalEnemiesRequired = 0;
    previousRoomsKilled = 0;
    totalKillTarget = 0;
    procWaveTimer = 0.0f;
    procWaveNumber = 0;
    bossMaxHPProgressBar = 100.f;
    bossHPProgressBar = 0.f;
    inProceduralMap = false;

    // Shared player setup used by all three modes
    Bitmask collideMask = CreateBitmask(3,
        Collision::LAYER::ENEMIES,
        Collision::LAYER::INTERACTABLE,
        Collision::LAYER::OBSTACLE
    );

    float playerRadius = GameDB::GetPlayerRadius();

    ActorStats base{};
    base.maxHP = 100.0f;
    base.attack = 10.0f;
    base.attackSpeed = 1.0f;
    base.moveSpeed = playerSpeed;

    // ── ENDLESS MODE ──────────────────────────────────────────
    // No CSV map — generate a proc room and drop the player straight in
    if (mapSelected == "Assets/Endless.csv") {
        std::cout << "[InitState] Endless mode.\n";

        totalKillTarget = 20 + rand() % 31;
        totalEnemiesRequired = totalKillTarget;
        std::cout << "[InitState] Kill target: " << totalKillTarget << "\n";

        csvEnemies.clear();
        nextMap->GenerateProcedural(50, 50, rand());
        AEVec2 procSpawn = nextMap->GetSpawnPoint();

        gPlayer->Init(procSpawn,
            AEVec2{ playerRadius * 2.f, playerRadius * 2.f },
            0, MESH_CIRCLE, Collision::SHAPE::COL_CIRCLE,
            AEVec2{ playerRadius * 2.f, playerRadius * 2.f },
            collideMask, Collision::LAYER::PLAYER);

        gPlayer->GetRenderData().AddTexture(GameDB::GetPlayerTexturePath());
        gPlayer->GetRenderData().SetActiveTexture(0);

        PetManager::GetInstance()->InitPetForGame(*nextMap);
        gPlayer->InitPlayerRuntime(base);
        gPlayer->ApplyShopUpgrades();
        gPlayer->Heal(gPlayer->GetMaxHP());

        camPos = procSpawn;
        camVel = { 0, 0 };
        halfMapWidth = nextMap->GetFullMapSize().x * 0.5f;
        halfMapHeight = nextMap->GetFullMapSize().y * 0.5f;
        SetCameraPos(camPos);
        inProceduralMap = true;

        procWaveNumber = 0;
        SpawnProcWave(*nextMap);
        SpawnProcChests(*nextMap);   // random 1-3 chests in the proc room
        procWaveTimer = 0.0f;
        loadingTimer = LOADING_DURATION; // show loading screen while assets settle
        endlessRunTimer = 0.f;
        endlessTimerActive = true;

        PetManager::GetInstance()->SetTilemap(*nextMap);
        minimap->Reset();
        return;
    }

    // ── TUTORIAL and NORMAL — find safe spawn in CSV map ─────
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



    gPlayer->Init(safeSpawnPos,
        AEVec2{ playerRadius * 2.f, playerRadius * 2.f },
        0, MESH_CIRCLE, Collision::SHAPE::COL_CIRCLE,
        AEVec2{ playerRadius * 2.f, playerRadius * 2.f },
        collideMask, Collision::LAYER::PLAYER);

    gPlayer->GetRenderData().AddTexture(GameDB::GetPlayerTexturePath());
    gPlayer->GetRenderData().SetActiveTexture(0);

    PetManager::GetInstance()->InitPetForGame(*map);
    gPlayer->InitPlayerRuntime(base);
    gPlayer->ApplyShopUpgrades();
    gPlayer->Heal(gPlayer->GetMaxHP());

    // Spawn all chests the designer placed in the CSV map
    SpawnCsvChests(*map);

    camPos = safeSpawnPos;
    camVel = { 0, 0 };
    minimap->Reset();

    // ── TUTORIAL MODE ─────────────────────────────────────────
    // Spawn CSV enemies, find the door tile, no kill target or proc rooms
    if (doTutorial) {
        std::cout << "[InitState] Tutorial mode.\n";

        // Find the TILE_DOOR position so the fairy can guide to it
        // and the player can walk through after the boss dies
        for (unsigned r = 0; r < csvRows; ++r) {
            for (unsigned c = 0; c < csvCols; ++c) {
                const TileMap::Tile* t = map->QueryTile(r, c);
                if (t && t->type == TileMap::TILE_DOOR) {
                    currentLevel.doorPos = map->GetTilePosition(r, c);
                    std::cout << "[Tutorial] Door found at ("
                        << currentLevel.doorPos.x << ", "
                        << currentLevel.doorPos.y << ")\n";
                    break;
                }
            }
        }
        
        // Boss spawn in tutorial 
        for (unsigned r = 0; r < csvRows; ++r) {
            for (unsigned c = 0; c < csvCols; ++c) {
                const TileMap::Tile* t = map->QueryTile(r, c);
                if (t && t->type == TileMap::TILE_BOSS) {
                    AEVec2 bossSpawnPos = map->GetTilePosition(r, c);
                    boss = SpawnRandomBossEnemy(bossSpawnPos);
                    if (boss) {
                        bossSpawned = true;
                        bossAlive = true;
                        bossMaxHPProgressBar = boss->GetDefinition().baseStats.maxHP;
                        bossHPProgressBar = bossMaxHPProgressBar;
                        boss->SetEnabled(false); // hidden until fairy reaches BOSS stage
                    }
                    break;
                }
            }
        }

        std::vector<AEVec2> csvSpawnPool = FindSafeSpawnPositions(*map, 0, true);
        csvEnemies.clear();
        for (AEVec2 const& pos : csvSpawnPool) {
            Enemy* e = SpawnWeightedEnemy(pos, 0.70f, 0.30f);
            if (!e) continue;
            csvEnemies.push_back(e);
            std::cout << "[Tutorial] Spawned " << e->GetDefinition().name
                << " at (" << pos.x << ", " << pos.y << ")\n";
        }
        std::cout << "[Tutorial] Spawned " << csvEnemies.size() << " enemies total.\n";
        if (doTutorial) {
            fairy->InitTutorial(gPlayer, *map, currentLevel);
            fairy->tilemap = map;   // give fairy access to tilemap for door detection
        }
        return;
    }

    // ── NORMAL MODE ───────────────────────────────────────────
    // CSV enemies first, then proc rooms after walking through connector
    totalKillTarget = 20 + rand() % 31;
    totalEnemiesRequired = totalKillTarget;
    std::cout << "[InitState] Kill target: " << totalKillTarget << "\n";

    std::vector<AEVec2> csvSpawnPool = FindSafeSpawnPositions(*map, 0, true);
    csvEnemies.clear();

    std::cout << "[InitState] csvSpawnPool has " << csvSpawnPool.size() << " positions.\n";

    for (AEVec2 const& pos : csvSpawnPool) {
        Enemy* e = SpawnWeightedEnemy(pos, 0.70f, 0.30f);
        if (!e) {
            std::cout << "[InitState] SpawnWeightedEnemy null at ("
                << pos.x << ", " << pos.y << ")\n";
            continue;
        }
        csvEnemies.push_back(e);
        const char* tier = (e->GetDefinition().category == EnemyCategory::Elite)
            ? "Elite" : "Normal";
        std::cout << "[InitState] Spawned " << e->GetDefinition().name
            << " (" << tier << ") at (" << pos.x << ", " << pos.y << ")\n";
    }
    std::cout << "[InitState] Spawned " << csvEnemies.size() << " CSV enemies total.\n";
}

// =============================================================
void GameState::Update(double dt)
{
    // ESC or M toggles the pause menu
    if (loadingTimer <= 0.f &&
        (AEInputCheckTriggered(AEVK_M) || AEInputCheckTriggered(AEVK_ESCAPE)))
    {
        Pause::Toggle();
        return;
    }

    // Pause menu consumes all game logic while open
    if (Pause::Update()) return;

    // Show loading screen — skip all game logic until timer expires
    if (loadingTimer > 0.f) {
        loadingTimer -= (float)dt;
        if (loadingTimer < 0.f) loadingTimer = 0.f;
        return; // don't process any gameplay input or updates
    }

    // TAB — toggle debug mode and overlay
    if (AEInputCheckTriggered(AEVK_TAB)) {
        debugMode = !debugMode;
        showDebugOverlay = debugMode;
        std::cout << "[Debug] Mode " << (debugMode ? "ON" : "OFF") << "\n";
    }

    if (AEInputCheckTriggered(AEVK_K))
        showKeybindOverlay = !showKeybindOverlay;

    if (debugMode) {
        if (AEInputCheckTriggered(AEVK_F5)) {
            debugGodMode = !debugGodMode;
            std::cout << "[Debug] God mode " << (debugGodMode ? "ON" : "OFF") << "\n";
        }
        if (AEInputCheckTriggered(AEVK_F6)) {
            debugFreezeEnemies = !debugFreezeEnemies;
            std::cout << "[Debug] Freeze AI " << (debugFreezeEnemies ? "ON" : "OFF") << "\n";
        }
        if (AEInputCheckTriggered(AEVK_F7)) {
            debugShowChests = !debugShowChests;
            std::cout << "[Debug] Show chests " << (debugShowChests ? "ON" : "OFF") << "\n";
        }
        if (AEInputCheckTriggered(AEVK_F1)) {
            for (Enemy* e : procEnemies) if (e && e->IsEnabled()) e->TakeDamage({ 99999.f, nullptr, DAMAGE_TYPE::TRUE_DAMAGE, nullptr });
            for (Enemy* e : csvEnemies)  if (e && e->IsEnabled()) e->TakeDamage({ 99999.f, nullptr, DAMAGE_TYPE::TRUE_DAMAGE, nullptr });
            std::cout << "[Debug] All enemies killed.\n";
        }
        if (AEInputCheckTriggered(AEVK_F2)) {
            if (!bossSpawned && inProceduralMap && nextMap) {
                totalEnemiesKilled = totalKillTarget;
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
        LootChest* chest = dynamic_cast<LootChest*>(
            GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
        AEVec2 m = GetMouseWorldVec();
        chest->Init(m, { 75,75 }, 1, MESH_SQUARE, Collision::COL_RECT, { 75,75 },
            CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE);
    }
    if (AEInputCheckTriggered(AEVK_R))
        PostOffice::GetInstance()->Send("PetManager", new PetSkillMsg(PetSkillMsg::CAST_SKILL));

    if (AEInputCheckTriggered(AEVK_N)) {
        AEVec2 m = GetMouseWorldVec();
        Enemy* e = SpawnWeightedEnemy(m, 0.70f, 0.30f);
        if (e) {
            const char* tier = (e->GetDefinition().category == EnemyCategory::Elite) ? "Elite" : "Normal";
            std::cout << "[Spawn N] " << e->GetDefinition().name << " (" << tier << ")\n";
            if (inProceduralMap) procEnemies.push_back(e);
            else                 csvEnemies.push_back(e);
            ++enemiesRequiredForBoss;
        }
    }
#pragma endregion

    if (!gPlayer) return;

    if (debugMode && debugGodMode)
        gPlayer->Heal(gPlayer->GetMaxHP());

    if (teleportCooldown > 0.f) teleportCooldown -= (float)dt;

    // Proc wave timer — only ticks in proc map before boss spawns.
    // Wave 0 is already spawned on room entry so this handles wave 1+.
    if (inProceduralMap && !bossSpawned && nextMap) {
        procWaveTimer += (float)dt;
        if (procWaveTimer >= PROC_WAVE_INTERVAL) {
            procWaveTimer = 0.0f;
            SpawnProcWave(*nextMap);
        }
    }

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
            previousRoomsKilled = totalEnemiesKilled;
            // Clear CSV enemies so they don't bleed into the proc map
            DisableAndClearEnemies(csvEnemies);
            DisableAndClearEnemies(procEnemies);
            procWaveNumber = 0;
            // Wave 0 fires immediately — 8 enemies
            SpawnProcWave(*nextMap);
            // Random 1-3 chests for this proc room
            SpawnProcChests(*nextMap);
            procWaveTimer = 0.0f;
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
            previousRoomsKilled = totalEnemiesKilled;
            // Clear previous room's enemies before spawning new ones
            DisableAndClearEnemies(procEnemies);
            procWaveNumber = 0;
            // Wave 0 fires immediately — 8 enemies
            SpawnProcWave(*nextMap);
            // Random 1-3 chests for this proc room
            SpawnProcChests(*nextMap);
            procWaveTimer = 0.0f;
            minimap->Reset();
        }
    }

    TileMap* currentMap = inProceduralMap ? nextMap : map;

    AEVec2 move = gPlayer->GetMoveDirNorm();
    f32 len = AEVec2Length(&move);
    if (len > 0 || gPlayer->HasForceApplied())
        playerDir = gPlayer->GetMoveDirNorm();

    // Count dead enemies every frame to keep the progress bar accurate
    if (!(debugMode && debugFreezeEnemies))
        CountAllDeadEnemies();

    if (inProceduralMap)
        TrySpawnBoss(*nextMap);

    if (boss && bossSpawned) {
        bossHPProgressBar = (boss->GetHP() / boss->GetMaxHP()) * bossMaxHPProgressBar;
        bossAlive = !boss->IsDead();
    }

    if (doTutorial && fairy->data.stage == Tutorial::BOSS) {
        if (boss && !boss->IsDead())
            boss->SetEnabled(true);
        if (!bossAlive)
            fairy->ChangeStage(Tutorial::END);
    }

    if (endlessTimerActive) {
        endlessRunTimer += (float)dt;
    }

    // Non-tutorial: return to main menu when boss is slain.
    if (!doTutorial && bossSpawned && !bossAlive) {
        if (mapSelected != "Assets/Endless.csv") {
            GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
            std::cout << "BOSS SLAYED\n";
        }
        else {
            // Endless — reset boss state and spawn a new proc room to continue
            std::cout << "BOSS SLAYED — Endless continues!\n";
            bossSpawned = false;
            bossAlive = true;
            boss = nullptr;
            bossHPProgressBar = 0.f;
            bossMaxHPProgressBar = 100.f;

            // Reset kill target and all counters for the next boss cycle
            totalKillTarget = 20 + rand() % 31;
            totalEnemiesRequired = totalKillTarget;
            previousRoomsKilled = 0;
            totalEnemiesKilled = 0;
            enemiesKilledInRoom = 0;
            // Clear dead enemies so they don't count toward the new cycle
            DisableAndClearEnemies(procEnemies);
            std::cout << "[Endless] New kill target: " << totalKillTarget << "\n";

            teleportCooldown = 2.f;
            if (nextMap) {
                nextMap->GenerateProcedural(50, 50, rand());
                procWaveNumber = 0;
                SpawnProcWave(*nextMap);
                SpawnProcChests(*nextMap);
                procWaveTimer = 0.0f;
                minimap->Reset();
                PetManager::GetInstance()->SetTilemap(*nextMap);
            }
        }
    }
    if (mapSelected == "Assets/Endless.csv" && gPlayer && gPlayer->GetHP() <= 0.f) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
        std::cout << "PLAYER DIED — Endless over.\n";
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

    DropSystem::UpdatePickupDisplay(static_cast<float>(dt));
}

// =============================================================
void GameState::Draw()
{
    // Loading screen overlay — drawn instead of the game world during load
    if (loadingTimer > 0.f) {
        float winW = (float)(AEGfxGetWinMaxX() - AEGfxGetWinMinX());
        float winH = (float)(AEGfxGetWinMaxY() - AEGfxGetWinMinY());

        // Full-screen black background
        AEMtx33 bgMtx;
        GetTransformMtx(bgMtx, { 0, 0 }, 0, { winW, winH });
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);
        AEGfxSetTransform(bgMtx.m);
        AEGfxSetColorToMultiply(0.f, 0.f, 0.f, 1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // Progress bar background (dark grey, centered)
        float barW = winW * 0.5f, barH = 24.f;
        float filled = (1.f - loadingTimer / LOADING_DURATION) * barW;
        AEMtx33 barBgMtx, barFillMtx;
        GetTransformMtx(barBgMtx, { 0.f, -60.f }, 0, { barW, barH });
        GetTransformMtx(barFillMtx, { -barW * 0.5f + filled * 0.5f, -60.f }, 0, { filled, barH });
        AEGfxSetTransform(barBgMtx.m);
        AEGfxSetColorToMultiply(0.2f, 0.2f, 0.2f, 1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
        AEGfxSetTransform(barFillMtx.m);
        AEGfxSetColorToMultiply(0.2f, 0.7f, 1.f, 1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // "LOADING..." text
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        if (font >= 0)
            AEGfxPrint(font, "LOADING...", -0.12f, 0.05f, 1.2f, 1.f, 1.f, 1.f, 1.f);

        return; // skip all other rendering while loading
    }

    RenderWorldMap();
    GameObjectManager::GetInstance()->DrawObjects();
    DrawBossHPProgressBar();

    TileMap* currentMap = inProceduralMap ? nextMap : map;
    minimap->Render(*currentMap, *gPlayer);

    // Enemy HP labels always visible
    DrawEnemyStats(MakeDebugCtx());
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    if (debugMode) {
        DebugContext dbg = MakeDebugCtx();
        // Draw chest locations as yellow dots on the minimap
        if (dbg.debugShowChests) {
            TileMap* currentMapForChests = inProceduralMap ? nextMap : map;
            AEVec2 const& mapSize = currentMapForChests->GetFullMapSize();
            float scaleX = 200.f / mapSize.x;
            float scaleY = 200.f / mapSize.y;
            float mmX = (float)AEGfxGetWinMaxX() - 200.f * 0.5f - 20.f;
            float mmY = (float)AEGfxGetWinMaxY() - 200.f * 0.5f - 20.f - 10.f;
            AEVec2 mapOffset = currentMapForChests->GetOffset();

            auto& gos = GameObjectManager::GetInstance()->GetGameObjects();
            for (GameObject* go : gos) {
                if (!go || !go->IsEnabled() || go->GetGOType() != GO_TYPE::LOOT_CHEST) continue;
                AEVec2 relPos = go->GetPos() - mapOffset;
                AEVec2 dotPos = { mmX + relPos.x * scaleX, mmY + relPos.y * scaleY };
                AEMtx33 mtx;
                GetTransformMtx(mtx, dotPos, 0, { 10.f, 10.f });
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxTextureSet(nullptr, 0, 0);
                AEGfxSetTransform(mtx.m);
                AEGfxSetColorToMultiply(1.0f, 0.85f, 0.0f, 1.0f);
                AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        if (showDebugOverlay) DrawDebugOverlay(dbg);
    }

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    if (showKeybindOverlay) DrawKeybindOverlay(MakeDebugCtx());

    if (gPlayer) gPlayer->DrawUI();

    // Draw the Endless mode survival timer at the top center
    if (endlessTimerActive && font >= 0) {
        int minutes = (int)endlessRunTimer / 60;
        int seconds = (int)endlessRunTimer % 60;
        char timeText[64];
        snprintf(timeText, sizeof(timeText), "SURVIVED: %02d:%02d", minutes, seconds);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxPrint(font, timeText, -0.4f, 0.75f, 1.0f, 1.f, 1.f, 1.f, 1.f);
    }

    HandleTutorialDialogueRender();
    PetManager::GetInstance()->DrawUI();

    DropSystem::PrintPickupDisplay();
    // Pause menu drawn on top of everything
    Pause::Draw();
}

void GameState::HandleTutorialDialogueRender()
{
    if (!doTutorial || !fairy || !fairy->data.playDialogue) return;
    DrawAEText(font, fairy->data.dialogueLines[fairy->data.currDialogueLine].c_str(),
        fairy->data.dialoguePos, fairy->data.dialogueSize,
        CreateColor(238, 128, 238, 255), TEXT_MIDDLE, 1);
}

// =============================================================
void GameState::ExitState()
{
    // Flush coins and endless timer before destroying things
    if (gPlayer) {
        ShopFunctions::GetInstance()->addMoney(gPlayer->GetInventory().GetCoins());
        gPlayer->GetInventory().Clear(); // clear to prevent any weird persistence issues later
    }
    if (endlessTimerActive) {
        ShopFunctions::GetInstance()->updateEndlessHighScore(endlessRunTimer);
        endlessTimerActive = false;
    }

    bgm.StopGameplayBGM();

    gPlayer->ClearStatusEffects();
    inProceduralMap = false;
    teleportCooldown = 0.f;
    bossSpawned = false;
    bossAlive = true;
    enemiesKilledInRoom = 0;
    enemiesRequiredForBoss = 0;
    totalEnemiesKilled = 0;
    totalEnemiesRequired = 0;
    previousRoomsKilled = 0;
    totalKillTarget = 0;
    loadingTimer = 0.f;
    procWaveTimer = 0.0f;
    procWaveNumber = 0;
    bossHPProgressBar = 0.f;
    bossMaxHPProgressBar = 100.f;
    doTutorial = false;
    boss = nullptr;
    DisableAndClearEnemies(csvEnemies);
    DisableAndClearEnemies(procEnemies);

    debugMode = false;
    showDebugOverlay = false;
    showKeybindOverlay = false;
    debugGodMode = false;
    debugFreezeEnemies = false;
    debugShowChests = false;
}

// =============================================================
void GameState::UnloadState()
{
    if (wallMesh) AEGfxMeshFree(wallMesh);

    delete map;     map = nullptr;
    delete nextMap; nextMap = nullptr;
    delete minimap; minimap = nullptr;

    gPlayer = nullptr;
    boss = nullptr;
    DisableAndClearEnemies(csvEnemies);
    DisableAndClearEnemies(procEnemies);

    bgm.Exit();
    if (font >= 0) { AEGfxDestroyFont(font); font = -1; }
}

bool  getBossAlive() { return bossAlive; }
float getBossHPProgressBar() { return bossHPProgressBar; }
void  setBossHPProgressBar(float v) { bossHPProgressBar = v; }
float getBossMaxHPProgressBar() { return bossMaxHPProgressBar; }
void  setBossMaxHPProgressBar(float v) { bossMaxHPProgressBar = v; }