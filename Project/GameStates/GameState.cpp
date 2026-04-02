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
#include <fstream>
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
#include "../Drops/PickupGO.h"
#include "../Debug.h"
#include "../GameStates/LevelSelectState.h"
#include "../ShopFunctions.h"
#include "../Pause.h"
#include "../GameEnd.h"
#include "../UI/UIElement.h"
#include <json/json.h>

namespace {
    // meshes
    AEGfxVertexList* circleMesh = nullptr;
    AEGfxVertexList* wallMesh = nullptr;
    AEGfxVertexList* squareMesh = nullptr;

    // player stuff
    AEVec2  playerPos;
    AEVec2  playerDir = { 1.0f, 0.0f };
    Player* gPlayer = nullptr;
    float   playerSpeed = 300.0f;

    static AEVec2 GetPlayerPos()
    {
        return gPlayer ? gPlayer->GetPos() : AEVec2{ 0.0f, 0.0f };
    }

    struct {
        //-----Healthbar----
        AEVec2 hpBarSize{};
        Color hpBgCol{}, hpFillCol{}, hpShieldCol{};
        float seIconSize{};
        unsigned maxIcons{};

        //----Inventory----
        AEVec2 invPos, invSize;
        Color invBGCol{}, invCoinCol, invAmmoCol, invStatTxtCol;
        AEVec2 invGearIconSz{}, invStatIconSz{};
        AEVec2 invEdgePadding{};
        float invIconGap, invSectionGap, invStatFontSz, invStatGap;

        // Not loaded from json
        AEVec2 hpBarPos{};
        AEMtx33 hpBarTrans{};
    } playerUISettings;

    std::array<UIElement*, 7> invGearElements{};
    int invGearHoverInd{ -1 };

    // --- BOSS / ENEMY TRACKING ---
    bool   bossAlive = true;
    bool   bossSpawned = false;
    float  bossRadius = 60.0f;
    Enemy* boss = nullptr;

    std::vector<Enemy*> csvEnemies;
    std::vector<Enemy*> procEnemies;

    int   enemiesKilledInRoom = 0;
    int   enemiesRequiredForBoss = 0;

    // kill progress for the top bar
    // totalKillTarget is picked randomly between (20-39) once per run and never changes for endless mode
    // For nomal mode is it picked randomly between (14-20) 
    // previousRoomsKilled snapshots the count before we wipe the enemy lists on room transition
    // so the bar doesn't jump back to 0
    int   totalEnemiesKilled = 0;
    int   totalEnemiesRequired = 0;
    int   previousRoomsKilled = 0;
    int   totalKillTarget = 0;

    float bossSpawnThreshold = 1.0f;
    float bossRespawnTimer = 0.f;
    bool  bossRespawnPending = false;
    const float BOSS_RESPAWN_DELAY = 3.f;

    // enemy waves in proc rooms
    // first wave (wave 0) fires straight away with 12 enemies when you enter
    // after that one more enemy is added per wave: wave 1=5, wave 2=6, etc.
    float procWaveTimer = 0.0f;
    int   procWaveNumber = 0;
    const float PROC_WAVE_INTERVAL = 4.0f;

    // chest waves in proc rooms
    // 4 chests drop on room entry, then 1 more every CHEST_WAVE_INTERVAL seconds
    // no hard cap — they keep coming until the boss spawns
    float procChestWaveTimer = 0.0f;
    int   procChestWaveNumber = 0;
    const float CHEST_WAVE_INTERVAL = 15.0f;

    // camera
    AEVec2 camPos, camVel;
    float  camZoom = 1.2f;
    float  camSmoothTime = 0.15f;

    // map stuff
    float    mapWidth = 2400.0f;
    float    mapHeight = 2000.0f;
    float    halfMapWidth, halfMapHeight;
    MapData  currentLevel;
    TileMap* map{};
    TileMap* nextMap{};
    Minimap* minimap{};
    float    teleportCooldown = 0.f;
    bool     inProceduralMap = false;

    // tutorial
    bool doTutorial{ true };
    Tutorial::TutorialFairy* fairy;
    s8 font{ -1 };

    // boss HP bar at the top of screen (also doubles as kill progress before boss spawns)
    float bossHPProgressBarWidth = 0.f;
    float bossHPProgressBarHeight = 0.f;
    float bossHPProgressBar = 100.f;
    float bossMaxHPProgressBar = 100.f;

    // debug toggles
    bool debugMode = false;
    bool showDebugOverlay = false;
    bool showKeybindOverlay = false;
    bool debugGodMode = false;
    bool debugFreezeEnemies = false;
    bool debugShowChests = false;   // F7 — shows chest dots on minimap

    // loading screen counts down from LOADING_DURATION, game logic is paused while it's > 0
    float loadingTimer = 0.f;
    const float LOADING_DURATION = 7.f;

    // endless mode survival clock
    float endlessRunTimer = 0.f;
    bool endlessTimerActive = false;

    float screenFlashTimer = 0.0f;
    float maxFlashDuration = 0.5f;

    // keyboard popup — shown once on first non-tutorial run, toggled with X after that
    AEGfxTexture* keyboardTex = nullptr;
    bool showKeyboardMenu = false;
    bool keyboardShownOnce = false;

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

    // Grabs spawn positions from the tilemap.
    // On CSV maps we just trust the designer-placed TILE_ENEMY markers.
    // On proc maps those markers need to pass a clearance check first.
    // Falls back to scanning open tiles if no markers are found at all.
    std::vector<AEVec2> FindSafeSpawnPositions(TileMap const& tilemap, int maxCount, bool trustMarkers = true)
    {
        std::vector<AEVec2> positions;
        auto mapSz = tilemap.GetMapSize();
        unsigned cols = mapSz.first;
        unsigned rows = mapSz.second;

        // first pass: look for TILE_ENEMY markers
        for (unsigned r = 0; r < rows; ++r) {
            for (unsigned c = 0; c < cols; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (!t || t->type != TileMap::TILE_ENEMY) continue;
                if (!trustMarkers && !tilemap.HasClearance(r, c, 3)) continue;
                positions.push_back(tilemap.GetTilePosition(r, c));
                std::cout << "[FindSafe] TILE_ENEMY at (" << r << "," << c << ") accepted.\n";
            }
        }
        if (!positions.empty()) return positions;

        std::cout << "[FindSafe] No TILE_ENEMY markers — falling back to open tile scan.\n";

        // no markers, so just scan the map for open tiles with enough space around them
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

    // Builds a pool of valid spawn positions for the proc map.
    // Skips walls, the centre (where the player spawns), and anything too close to a corridor.
    // clearance param lets callers ask for a tighter bubble — chests use 2, enemies use 1.
    std::vector<AEVec2> CollectProcSafePositions(TileMap const& tilemap, int clearance = 1)
    {
        std::vector<AEVec2> positions;
        auto mapSz = tilemap.GetMapSize();
        unsigned cols = mapSz.first;
        unsigned rows = mapSz.second;
        int midR = (int)rows / 2;
        int midC = (int)cols / 2;

        for (unsigned r = 6; r < rows - 6; ++r) {
            for (unsigned c = 5; c < cols - 5; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (!t || t->type != TileMap::TILE_NONE || t->isSolid) continue;
                if (!tilemap.HasClearance(r, c, clearance)) continue;

                int dr = (int)r - midR, dc = (int)c - midC;
                if (dr >= -5 && dr <= 5 && dc >= -5 && dc <= 5) continue;
                if ((int)r == midR || (int)c == midC) continue;

                positions.push_back(tilemap.GetTilePosition(r, c));
            }
        }
        std::cout << "[CollectProc] Found " << positions.size() << " safe positions.\n";
        return positions;
    }

    // Reads TILE_CHEST markers from the CSV and plops a chest on each one.
    // Only for CSV maps — proc maps use SpawnProcChestWave instead.
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

                // tutorial has its own drop table with beginner-friendly loot
                if (doTutorial) {
                    chest->SetDropTable(2);
                }
            }
        }
    }

    // Spawns `count` chests in the proc room (default 1).
    // Pass 4 on room entry for the opening burst, then the timer calls this
    // with count=1 every 15 seconds. Checks spacing against every live chest
    // so they don't all pile up in the same corner.
    void SpawnProcChestWave(TileMap const& tilemap, int count = 1)
    {
        std::vector<AEVec2> safePool = CollectProcSafePositions(tilemap, 2);
        if (safePool.empty()) return;

        const float MIN_CHEST_SPACING = 115.f * 5.f;

        // grab positions of chests already in the world so we keep them spread out
        std::vector<AEVec2> placed;
        for (GameObject* go : GameObjectManager::GetInstance()->GetGameObjects()) {
            if (go && go->IsEnabled() && go->GetGOType() == GO_TYPE::LOOT_CHEST)
                placed.push_back(go->GetPos());
        }

        int spawned = 0;
        for (int i = 0; i < count && !safePool.empty(); ++i) {
            bool found = false;
            for (int attempt = 0; attempt < 30 && !safePool.empty(); ++attempt) {
                int    idx = rand() % (int)safePool.size();
                AEVec2 pos = safePool[idx];
                safePool.erase(safePool.begin() + idx);

                // make sure the 2 tiles around the spot are clear, not just the tile itself
                AEVec2 gridInd = tilemap.GetTileIndFromPos(pos);
                int r = (int)gridInd.y, c = (int)gridInd.x;
                bool strictlySafe = true;
                for (int dr = -2; dr <= 2 && strictlySafe; ++dr)
                    for (int dc = -2; dc <= 2 && strictlySafe; ++dc) {
                        const TileMap::Tile* n = tilemap.QueryTile(r + dr, c + dc);
                        if (!n || n->type != TileMap::TILE_NONE) strictlySafe = false;
                    }
                if (!strictlySafe) continue;

                // reject if it's too close to a chest we already placed
                bool tooClose = false;
                for (AEVec2 const& cp : placed) {
                    float dx = pos.x - cp.x, dy = pos.y - cp.y;
                    if ((dx * dx + dy * dy) < MIN_CHEST_SPACING * MIN_CHEST_SPACING) {
                        tooClose = true; break;
                    }
                }
                if (tooClose) continue;

                LootChest* chest = dynamic_cast<LootChest*>(
                    GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
                if (!chest) break;

                chest->Init(pos, { 32.f, 32.f }, 0, MESH_SQUARE,
                    Collision::COL_RECT, { 28.f, 28.f },
                    CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE);

                placed.push_back(pos); // track it so the next chest in this batch stays spaced too
                ++procChestWaveNumber;
                ++spawned;
                found = true;
                std::cout << "[ChestWave " << procChestWaveNumber << "] Spawned chest at ("
                    << pos.x << ", " << pos.y << ")\n";
                break;
            }
            if (!found) break; // ran out of valid spots
        }
        std::cout << "[ChestWave] Spawned " << spawned << " chest(s) this wave.\n";
    }

    // turns off all enemies in the list and empties it
    // so nothing bleeds into the next room
    void DisableAndClearEnemies(std::vector<Enemy*>& enemies)
    {
        for (Enemy* e : enemies)
            if (e && e->IsEnabled()) e->SetEnabled(false);
        enemies.clear();
    }

    // turns off every chest currently in the world
    // call this before spawning new ones on room transition, otherwise old chests hang around
    void DisableAndClearChests()
    {
        auto& gos = GameObjectManager::GetInstance()->GetGameObjects();
        for (GameObject* go : gos) {
            if (go && go->IsEnabled() && go->GetGOType() == GO_TYPE::LOOT_CHEST)
                go->SetEnabled(false);
        }
        std::cout << "[ClearChests] All chests disabled.\n";
    }

    // first wave is 12 enemies and fires the moment you step into a proc room
    // after that each wave is 1 bigger than the last: wave 1=5, wave 2=6, etc.
    // caps at 50 live enemies at once so it doesn't get insane
    // in endless mode enemies also scale 10% stronger per minute survived
    void SpawnProcWave(TileMap const& tilemap)
    {
        const int MAX_LIVE_ENEMIES = 50;
        const float MIN_SPAWN_SPACING = 115.f * 2.5f; // ~5 tiles gap between spawns

        // scale enemy stats up every 60 seconds in endless (10% per minute)
        float currentDifficulty = 1.0f;
        if (mapSelected == "Assets/Endless.csv") {
            currentDifficulty = 1.0f + (endlessRunTimer / 60.0f) * 0.10f;
        }

        // don't add more if we're already at the cap
        int liveCount = 0;
        std::vector<AEVec2> chosen;
        for (Enemy* e : procEnemies) {
            if (e && e->IsEnabled() && e->GetHP() > 0.f) {
                ++liveCount;
                chosen.push_back(e->GetPos());
            }
        }

        int canSpawn = MAX_LIVE_ENEMIES - liveCount;
        if (canSpawn <= 0) {
            std::cout << "[ProcWave] Live enemy cap hit (" << liveCount << "), skipping wave.\n";
            ++procWaveNumber; // still bump the number so the timer resets properly
            return;
        }

        int waveSize = (procWaveNumber == 0) ? 12 : (4 + procWaveNumber);
        waveSize = (waveSize < canSpawn) ? waveSize : canSpawn;
        ++procWaveNumber;

        std::vector<AEVec2> safePool = CollectProcSafePositions(tilemap, 1);
        if (safePool.empty()) return;

        int spawned = 0;
        for (int i = 0; i < waveSize && !safePool.empty(); ++i) {
            // try up to 20 picks per enemy slot to find a well-spaced one
            bool placed = false;
            for (int attempt = 0; attempt < 20 && !safePool.empty(); ++attempt) {
                int idx = rand() % (int)safePool.size();
                AEVec2 pos = safePool[idx];

                // skip if it's too close to something we already picked this wave
                bool tooClose = false;
                for (AEVec2 const& c : chosen) {
                    float dx = pos.x - c.x, dy = pos.y - c.y;
                    if ((dx * dx + dy * dy) < MIN_SPAWN_SPACING * MIN_SPAWN_SPACING) {
                        tooClose = true; break;
                    }
                }

                safePool.erase(safePool.begin() + idx);
                if (tooClose) continue;

                Enemy* e = SpawnWeightedEnemy(pos, 0.70f, 0.30f);
                if (e) {
                    // apply the difficulty multiplier so endless enemies get harder over time
                    e->mDifficultyMultiplier = currentDifficulty;
                    e->OnStatEffectChange();
                    e->Heal(e->GetStats().maxHP);

                    procEnemies.push_back(e);
                    chosen.push_back(pos);
                    ++spawned;
                    placed = true;
                    break;
                }
            }
            if (!placed) continue; // pool dried up
        }
    }

    // once the kill target is hit, wipe the remaining enemies and drop the boss in
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

        // Only in endless mode boss stats scale with time as well.
        if (mapSelected == "Assets/Endless.csv") {
            float bossDifficulty = 1.0f + (endlessRunTimer / 60.0f) * 0.10f;
            boss->mDifficultyMultiplier = bossDifficulty;
            boss->OnStatEffectChange();
            boss->Heal(boss->GetStats().maxHP);
            std::cout << "[Boss] Difficulty multiplier: " << bossDifficulty
                << "x (at " << (int)endlessRunTimer << "s)\n";
        }

        bossSpawned = true;
        bossAlive = true;
        bossMaxHPProgressBar = boss->GetDefinition().baseStats.maxHP * boss->mDifficultyMultiplier;
        bossHPProgressBar = bossMaxHPProgressBar;

        // clear out the regular enemies so the player is 1v1 with the boss
        DisableAndClearEnemies(procEnemies);
        DisableAndClearEnemies(csvEnemies);

        std::cout << "[Boss] Spawned " << boss->GetDefinition().name
            << "! (kill target was " << totalKillTarget << ")\n";
    }

    // tallies dead enemies from both lists every frame
    // previousRoomsKilled makes sure kills from old rooms aren't lost when we clear the lists
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

    // camera smoothly follows the player and stays clamped inside map bounds
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
        // show the exit door in green once the boss is dead
        if (!bossAlive) {
            DrawTintedMesh(
                GetTransformMtx({ currentLevel.doorPos.x, currentLevel.doorPos.y },
                    0, { 80, 80 }),
                wallMesh, nullptr, { 0, 0.8f * 255, 0, 255 }, 255);
        }
        if (inProceduralMap) { if (nextMap) nextMap->Render(); }
        else { map->Render(); }
    }

    // the bar at the top of the screen
    // green while you're still grinding toward the boss
    // switches to red when the boss is alive
    // goes grey when the boss is dead
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

    void DrawGearTooltip(EquipmentData const& eq) {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);

        s32 mX, mY;
        AEInputGetCursorPosition(&mX, &mY);

        float winW = (float)AEGfxGetWinMaxX() - AEGfxGetWinMinX();
        float winH = (float)AEGfxGetWinMaxY() - AEGfxGetWinMinY();
        float screenX = (float)mX - (winW * 0.5f);
        float screenY = (winH * 0.5f) - (float)mY;

        //Min width = fit whole name + padding*2
        float _tw, _th;
        AEGfxGetPrintSize(font, eq.name, 0.45f, &_tw, &_th);
        AEVec2 bgSize = { max(300.0f, _tw*0.5f*AEGfxGetWindowWidth() + 30.f), 220.0f};
        AEVec2 bgPos = { screenX + bgSize.x * 0.5f + 15.0f, screenY - bgSize.y * 0.5f - 15.0f };

        if (bgPos.x + bgSize.x * 0.5f > winW * 0.5f) bgPos.x = winW * 0.5f - bgSize.x * 0.5f;
        if (bgPos.y - bgSize.y * 0.5f < -winH * 0.5f) bgPos.y = -winH * 0.5f + bgSize.y * 0.5f;

        DrawTintedMesh(GetTransformMtx(bgPos, 0.0f, bgSize), squareMesh, nullptr, { 0, 0, 0, 220 }, 255);

        AEVec2 textPos = { bgPos.x - bgSize.x * 0.5f + 15.0f, bgPos.y + bgSize.y * 0.5f - 25.0f };
        Color rColor = GetRarityColor(eq.rarity);

        DrawAEText(font, eq.name, textPos, 0.45f, rColor, TEXT_MIDDLE_LEFT);
        textPos.y -= 30.0f;

        DrawAEText(font, ("Sell Price: " + std::to_string(eq.sellPrice)).c_str(), textPos, 0.35f, { 255, 215, 0, 255 }, TEXT_MIDDLE_LEFT);

        if (eq.element != Elements::ELEMENT_TYPE::NONE) {
            textPos.y -= 30.0f;
            DrawAEText(font, ("Element: " + Elements::GetElementName(eq.element)).c_str(), textPos, 0.35f, Elements::GetElementCol(eq.element), TEXT_MIDDLE_LEFT);
        }
        textPos.y -= 35.f;

        if (eq.mods.additive.maxHP != 0) { DrawAEText(font, ("Max HP: +" + std::to_string((int)eq.mods.additive.maxHP)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
        if (eq.mods.additive.attack != 0) { DrawAEText(font, ("Attack: +" + std::to_string((int)eq.mods.additive.attack)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
        if (eq.mods.additive.defense != 0) { DrawAEText(font, ("Defense: +" + std::to_string((int)eq.mods.additive.defense)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
        if (eq.mods.additive.moveSpeed != 0) { DrawAEText(font, ("Speed: +" + std::to_string((int)eq.mods.additive.moveSpeed)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
        if (eq.mods.additive.attackSpeed != 0) {
            std::string aSpdStr = std::to_string(eq.mods.additive.attackSpeed);
            aSpdStr = aSpdStr.substr(0, aSpdStr.find('.') + 3);
            DrawAEText(font, ("Atk Speed: +" + aSpdStr).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f;
        }
    }

    void DrawPlayerUI() {
        int fontSz = RenderingManager::GetInstance()->GetFontSize();
        if (gPlayer->ShowStatsUI()) {
            Inventory const& mInventory{ gPlayer->GetInventory() };

            //Draw background
            DrawTintedMesh(GetTransformMtx(playerUISettings.invPos, 0.0f, playerUISettings.invSize), squareMesh, nullptr, { 0, 0, 0, 180 }, 255);

            //Draw equipped gear
            EquipmentData const* held{ gPlayer->GetHeldWeaponData() };
            int ind{ mInventory.GetActiveWeaponIndex() };

            std::array<EquipmentData const*, 7> gear{
                mInventory.GetMainWeapon(0),mInventory.GetMainWeapon(1),mInventory.GetBow(),
                mInventory.GetArmor(ArmorSlot::Head), mInventory.GetArmor(ArmorSlot::Body),mInventory.GetArmor(ArmorSlot::Hands),mInventory.GetArmor(ArmorSlot::Feet)
            };

            for (int i{}; i < gear.size(); ++i) {
                AEMtx33 mtx{ GetTransformMtx(invGearElements[i]->GetPos(), 0, invGearElements[i]->GetSize()) };
                //Draw background
                DrawTintedMesh(mtx,
                    squareMesh, nullptr, (held && ind == i) ? Color{0,255,0,255} : Color{ 100,100,0,255 }, 200);

                if (!gear[i]) continue;
                //Draw item texure
                mtx = GetTransformMtx(invGearElements[i]->GetPos(), 0, invGearElements[i]->GetSize());
                DrawTintedMesh(mtx, squareMesh, RenderingManager::GetInstance()->LoadTexture(gear[i]->texturePath), GetRarityColor(gear[i]->rarity), 255);
            }

            //Draw the player's stats
            float yLineSpc{ playerUISettings.invStatFontSz * fontSz + playerUISettings.invStatGap };
            AEVec2 textPos{ AEGfxGetWinMinX()+ playerUISettings.invEdgePadding.x,
                invGearElements.back()->GetPos().y - playerUISettings.invGearIconSz.y * 0.5f - playerUISettings.invSectionGap };
            ActorStats const& mStats{ gPlayer->GetStats() };

            DrawAEText(font, ("Max HP: " + std::to_string((int)mStats.maxHP)).c_str(), textPos, playerUISettings.invStatFontSz, playerUISettings.invStatTxtCol, TEXT_MIDDLE_LEFT); textPos.y -= yLineSpc;
            DrawAEText(font, ("Att: " + std::to_string((int)mStats.attack)).c_str(), textPos, playerUISettings.invStatFontSz, playerUISettings.invStatTxtCol, TEXT_MIDDLE_LEFT); textPos.y -= yLineSpc;
            DrawAEText(font, ("Def: " + std::to_string((int)mStats.defense)).c_str(), textPos, playerUISettings.invStatFontSz, playerUISettings.invStatTxtCol, TEXT_MIDDLE_LEFT); textPos.y -= yLineSpc;
            DrawAEText(font, ("Spd: " + std::to_string((int)mStats.moveSpeed)).c_str(), textPos, playerUISettings.invStatFontSz, playerUISettings.invStatTxtCol, TEXT_MIDDLE_LEFT); textPos.y -= yLineSpc;
            std::string tSpd = std::to_string(mStats.attackSpeed);
            size_t spdLen = tSpd.length() > 4 ? 4 : tSpd.length();
            DrawAEText(font, ("Atk Spd: " + tSpd.substr(0, spdLen)).c_str(), textPos, playerUISettings.invStatFontSz, playerUISettings.invStatTxtCol, TEXT_MIDDLE_LEFT); textPos.y -= yLineSpc;
            
            //Coin Counter
            std::string coinText = "Coins: " + std::to_string(mInventory.GetCoins());
            DrawAEText(font, coinText.c_str(), textPos, playerUISettings.invStatFontSz, playerUISettings.invCoinCol, TEXT_MIDDLE_LEFT);textPos.y -= yLineSpc;
            // Ammo Counter
            std::string ammoText = "Ammo: " + std::to_string(mInventory.GetAmmo());
            DrawAEText(font, ammoText.c_str(), textPos, playerUISettings.invStatFontSz, playerUISettings.invAmmoCol, TEXT_MIDDLE_LEFT);
        }

        DrawTintedMesh(playerUISettings.hpBarTrans, squareMesh, nullptr, playerUISettings.hpBgCol, playerUISettings.hpBgCol.a);
        AEVec2 hpBarFillSize{ playerUISettings.hpBarSize.x * (gPlayer->GetHP() / gPlayer->GetMaxHP()), playerUISettings.hpBarSize.y };
        AEVec2 hpBarFillPos = playerUISettings.hpBarPos;
        hpBarFillPos.x -= (playerUISettings.hpBarSize.x - hpBarFillSize.x) * 0.5f;
        DrawTintedMesh(GetTransformMtx(hpBarFillPos, 0, hpBarFillSize),
            squareMesh, nullptr, playerUISettings.hpFillCol, playerUISettings.hpFillCol.a);
        if (gPlayer->GetShieldVal()) {
            float shieldFill{ playerUISettings.hpBarSize.x * min(gPlayer->GetShieldVal() / gPlayer->GetMaxHP(), 1.f) };
            DrawTintedMesh(GetTransformMtx(playerUISettings.hpBarPos - AEVec2{ (playerUISettings.hpBarSize.x - shieldFill) * 0.5f,0 }, 0, { shieldFill, playerUISettings.hpBarSize.y }),
                squareMesh, nullptr, playerUISettings.hpShieldCol, playerUISettings.hpShieldCol.a);
        }
        DrawAEText(font,
            std::string{ std::to_string((int)gPlayer->GetHP()) + (gPlayer->GetShieldVal() ? (" (+" + std::to_string((int)gPlayer->GetShieldVal()) + ")") : "")
            + " / " + std::to_string((int)gPlayer->GetMaxHP()) }.c_str(),
            playerUISettings.hpBarPos, playerUISettings.hpBarSize.y / fontSz, Color{ 0,0,0,255 }, TEXT_MIDDLE);

        gPlayer->DrawStatusEffectIcons(playerUISettings.seIconSize,
            playerUISettings.hpBarPos + AEVec2{ 0, playerUISettings.hpBarSize.y * 0.5f + playerUISettings.seIconSize * 0.5f },
            playerUISettings.maxIcons, true, true);

        PickupGO* mInteractablePickup{ gPlayer->GetNearestPickup() };
        if (mInteractablePickup && mInteractablePickup->IsEnabled() && mInteractablePickup->GetPayload().equipment)
        {
            const EquipmentData* eq = mInteractablePickup->GetPayload().equipment;
            std::string nameStr = std::string(eq->name);
            std::string promptStr = "[E] Swap   [C] Sell (" + std::to_string(eq->sellPrice) + " Coins)";

            AEVec2 itemPos = { 0.0f, -55.0f };
            DrawAEText(font, nameStr.c_str(), itemPos, 0.4f, { 255,255,255,255 }, TEXT_MIDDLE);
            itemPos.y -= 20.0f;
            DrawAEText(font, promptStr.c_str(), itemPos, 0.4f, { 255,255,255,255 }, TEXT_MIDDLE);
        }

        //Tooltip for SE
        std::map<std::string, StatEffects::StatusEffect*> statusEffectsDict{ gPlayer->GetStatusEffects() };
        for (auto it{ statusEffectsDict.rbegin() }; it != statusEffectsDict.rend(); ++it) {
            StatEffects::StatusEffect& se = *(*it).second;
            se.UpdateUI(true);
        }
    }


    //Hovering over equipment in inventory UI
    void DrawUIHoveredGearTooltip(int& ind) {
        if (ind == -1) return;
        //Cursor pos
        s32 mx{}, my{};
        AEInputGetCursorPosition(&mx, &my);
        AEVec2 mP = ScreenToWorld(AEVec2{ (float)mx, (float)my });

        //Tooltip text
        Inventory const& mInventory{ gPlayer->GetInventory() };
        std::array<EquipmentData const*, 7> arr{
               mInventory.GetMainWeapon(0),mInventory.GetMainWeapon(1),mInventory.GetBow(),
               mInventory.GetArmor(ArmorSlot::Head), mInventory.GetArmor(ArmorSlot::Body),mInventory.GetArmor(ArmorSlot::Hands),mInventory.GetArmor(ArmorSlot::Feet)
        };
        EquipmentData const* eq{arr[ind]};
        if (eq) {
            DrawGearTooltip(*eq);
        }
        //Reset hover state
        ind = -1;
    }

    void DrawHoveredItemStats() {
        GameObject* hovered = GameObjectManager::GetInstance()->QueryOnMouse();
        if (hovered && hovered->GetGOType() == GO_TYPE::ITEM) {
            PickupGO* pickup = dynamic_cast<PickupGO*>(hovered);
            if (pickup) {
                const PickupPayload& payload = pickup->GetPayload();
                if (payload.type == DropType::Equipment && payload.equipment) {
                    const EquipmentData* eq = payload.equipment;

                    if (eq) {
                        DrawGearTooltip(*eq);
                    }
                }
            }
        }
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

    //Player UI — load settings from json so designers can tweak without recompiling
    std::ifstream ifs{ "Assets/Data/ui.json", std::ios_base::binary };
    if (ifs.is_open()) {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;

        if (Json::parseFromStream(builder, ifs, &root, &errs))
        {
            if (root.isMember("player_healthbar")) {
                Json::Value ui = root["player_healthbar"];
                if (ui.isMember("size") && ui["size"].size() == 2) {
                    playerUISettings.hpBarSize = AEVec2{ ui["size"][0].asFloat(), ui["size"][1].asFloat() };
                }
                if (ui.isMember("bgCol") && ui["bgCol"].size() == 4) {
                    playerUISettings.hpBgCol = Color{ ui["bgCol"][0].asFloat(), ui["bgCol"][1].asFloat(),
                    ui["bgCol"][2].asFloat() ,ui["bgCol"][3].asFloat() };
                }
                if (ui.isMember("fillCol") && ui["fillCol"].size() == 4) {
                    playerUISettings.hpFillCol = Color{ ui["fillCol"][0].asFloat(), ui["fillCol"][1].asFloat(),
                    ui["fillCol"][2].asFloat() ,ui["fillCol"][3].asFloat() };
                }
                if (ui.isMember("shieldCol") && ui["shieldCol"].size() == 4) {
                    playerUISettings.hpShieldCol = Color{ ui["shieldCol"][0].asFloat(), ui["shieldCol"][1].asFloat(),
                    ui["shieldCol"][2].asFloat() ,ui["shieldCol"][3].asFloat() };
                }
                playerUISettings.maxIcons = ui.get("maxIcons", 6).asUInt();
                playerUISettings.seIconSize = ui.get("seIconSize", 30).asFloat();
            }
            if (root.isMember("player_inventory")) {
                Json::Value ui = root["player_inventory"];
                if (ui.isMember("bgCol") && ui["bgCol"].size() == 4) {
                    playerUISettings.invBGCol = Color{ ui["bgCol"][0].asFloat(), ui["bgCol"][1].asFloat(),
                    ui["bgCol"][2].asFloat() ,ui["bgCol"][3].asFloat() };
                }
                if (ui.isMember("statTxtCol") && ui["statTxtCol"].size() == 4) {
                    playerUISettings.invStatTxtCol = Color{ ui["statTxtCol"][0].asFloat(), ui["statTxtCol"][1].asFloat(),
                    ui["statTxtCol"][2].asFloat() ,ui["statTxtCol"][3].asFloat() };
                }
                if (ui.isMember("coinTxtCol") && ui["coinTxtCol"].size() == 4) {
                    playerUISettings.invCoinCol = Color{ ui["coinTxtCol"][0].asFloat(), ui["coinTxtCol"][1].asFloat(),
                    ui["coinTxtCol"][2].asFloat() ,ui["coinTxtCol"][3].asFloat() };
                }
                if (ui.isMember("ammoTxtCol") && ui["ammoTxtCol"].size() == 4) {
                    playerUISettings.invAmmoCol = Color{ ui["ammoTxtCol"][0].asFloat(), ui["ammoTxtCol"][1].asFloat(),
                    ui["ammoTxtCol"][2].asFloat() ,ui["ammoTxtCol"][3].asFloat() };
                }
                playerUISettings.invGearIconSz.x = playerUISettings.invGearIconSz.y = ui.get("iconSize", 75).asFloat();
                if (ui.isMember("edgePadding") && ui["edgePadding"].size() == 2) {
                    playerUISettings.invEdgePadding = AEVec2{ ui["edgePadding"][0].asFloat(), ui["edgePadding"][1].asFloat() };
                }
                playerUISettings.invIconGap = ui.get("iconGap", 10).asFloat();
                playerUISettings.invSectionGap = ui.get("sectionGap", 10).asFloat();
                playerUISettings.invStatIconSz.x = playerUISettings.invStatIconSz.y = ui.get("statIconSize", 30).asFloat();
                playerUISettings.invStatFontSz = ui.get("statFontSize", 1).asFloat();
                playerUISettings.invStatGap = ui.get("statGap", 5).asFloat();
            }
        }
    }
    else {
        std::cout << "UI Json failed to open in GameState\n";
    }
    playerUISettings.hpBarPos = { 0, AEGfxGetWinMinY() + playerUISettings.hpBarSize.y * 0.5f + 5 };
    playerUISettings.hpBarTrans = GetTransformMtx(playerUISettings.hpBarPos, 0, playerUISettings.hpBarSize);

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
    keyboardTex = RenderingManager::GetInstance()->LoadTexture("Assets/sprites/keyboard.png");

    srand(1234);
    nextMap->GenerateProcedural(procRows, procCols, 1234);

    // find the first walkable tile in the CSV so the player doesn't spawn inside a wall
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
    GameEnd::Hide();

    // reload the map in case the player switched mode and came back
    if (map) { delete map; map = nullptr; }
    if (mapSelected == "Assets/Endless.csv") {
        map = new TileMap({ 0.f, 0.f }, 115.f, 115.f);
    }
    else {
        map = new TileMap(mapSelected, { 0,0 }, 115.f, 115.f);
        std::cout << "[InitState] Reloaded map: " << mapSelected << "\n";
    }

    // recalculate bounds after reload
    mapWidth = map->GetFullMapSize().x + nextMap->GetFullMapSize().x;
    mapHeight = (map->GetFullMapSize().y > nextMap->GetFullMapSize().y)
        ? map->GetFullMapSize().y : nextMap->GetFullMapSize().y;
    halfMapWidth = mapWidth * 0.5f;
    halfMapHeight = mapHeight * 0.5f;

    // wipe everything before we start
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
    procChestWaveTimer = 0.0f;
    procChestWaveNumber = 0;
    bossMaxHPProgressBar = 100.f;
    bossHPProgressBar = 0.f;
    inProceduralMap = false;

    //Load Inventory UI variables — recalculate layout in case window was resized
    playerUISettings.invSize = { 2 * (playerUISettings.invEdgePadding.x + playerUISettings.invGearIconSz.x) + playerUISettings.invIconGap,
                7 * (RenderingManager::GetInstance()->GetFontSize() * playerUISettings.invStatFontSz) + 6 * playerUISettings.invStatGap
        + 4 * (playerUISettings.invGearIconSz.y) + 3 * playerUISettings.invIconGap + 2 * playerUISettings.invEdgePadding.y };
    playerUISettings.invPos = { AEGfxGetWinMinX() + playerUISettings.invSize.x * 0.5f,0 };

    float y{ playerUISettings.invSize.y * 0.5f - (playerUISettings.invEdgePadding.y + playerUISettings.invGearIconSz.y * 0.5f) };
    invGearElements[0] = new UIElement{
        AEVec2{playerUISettings.invPos.x - (playerUISettings.invIconGap + playerUISettings.invGearIconSz.x) * 0.5f, y},
        playerUISettings.invGearIconSz, 2, Collision::COL_RECT
    };
    invGearElements[1] = new UIElement{
        AEVec2{playerUISettings.invPos.x + (playerUISettings.invIconGap + playerUISettings.invGearIconSz.x) * 0.5f, y},
        playerUISettings.invGearIconSz, 2, Collision::COL_RECT
    };
    y -= playerUISettings.invIconGap + playerUISettings.invGearIconSz.y;
    invGearElements[2] = new UIElement{
        AEVec2{playerUISettings.invPos.x, y},
        playerUISettings.invGearIconSz, 2, Collision::COL_RECT
    };
    y -= playerUISettings.invIconGap + playerUISettings.invGearIconSz.y;
    invGearElements[3] = new UIElement{
        AEVec2{playerUISettings.invPos.x - (playerUISettings.invIconGap + playerUISettings.invGearIconSz.x) * 0.5f, y},
        playerUISettings.invGearIconSz, 2, Collision::COL_RECT
    };
    invGearElements[4] = new UIElement{
        AEVec2{playerUISettings.invPos.x + (playerUISettings.invIconGap + playerUISettings.invGearIconSz.x) * 0.5f, y},
        playerUISettings.invGearIconSz, 2, Collision::COL_RECT
    };
    y -= playerUISettings.invIconGap + playerUISettings.invGearIconSz.y;
    invGearElements[5] = new UIElement{
        AEVec2{playerUISettings.invPos.x - (playerUISettings.invIconGap + playerUISettings.invGearIconSz.x) * 0.5f, y},
        playerUISettings.invGearIconSz, 2, Collision::COL_RECT
    };
    invGearElements[6] = new UIElement{
        AEVec2{playerUISettings.invPos.x + (playerUISettings.invIconGap + playerUISettings.invGearIconSz.x) * 0.5f, y},
        playerUISettings.invGearIconSz, 2, Collision::COL_RECT
    };
    for (int i{}; i < invGearElements.size(); ++i) {
        invGearElements[i]->SetHoverCallback([i](bool) {invGearHoverInd = i; });
    }

    // pop up keyboard menu on first entry of any non-tutorial stage in a session
    if (!doTutorial && !keyboardShownOnce) {
        showKeyboardMenu = true;
        keyboardShownOnce = true;
    }
    else {
        showKeyboardMenu = false;
    }

    // player init is the same across all three modes
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
    // no CSV, just drop straight into a proc room
    if (mapSelected == "Assets/Endless.csv") {
        std::cout << "[InitState] Endless mode.\n";
        gPlayer->SetHeldWeapon(0);
        totalKillTarget = 20 + rand() % 20;
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
        procWaveTimer = 0.0f;
        procChestWaveNumber = 0;
        SpawnProcChestWave(*nextMap, 4);  // drop 4 chests as soon as the room loads
        procChestWaveTimer = 0.0f;

        loadingTimer = LOADING_DURATION;
        endlessRunTimer = 0.f;
        endlessTimerActive = true;

        // when player dies, alert game state so the end screen shows
        gPlayer->SubToOnDeath(this);

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

    gPlayer->InitPlayerRuntime(base);
    gPlayer->ApplyShopUpgrades();
    gPlayer->Heal(gPlayer->GetMaxHP());
    // pets don't follow in tutorial so the player isn't overwhelmed early on
    if (!doTutorial) {
        PetManager::GetInstance()->InitPetForGame(*map);
    }
    else {
        PetManager::GetInstance()->UnsetPet();
    }

    // place all the designer-authored chests from the CSV
    SpawnCsvChests(*map);

    camPos = safeSpawnPos;
    camVel = { 0, 0 };
    minimap->Reset();

    // ── TUTORIAL MODE ─────────────────────────────────────────
    if (doTutorial) {
        std::cout << "[InitState] Tutorial mode.\n";
        gPlayer->SetHeldWeapon(0);

        // find the door so the fairy knows where to guide the player after the boss dies
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

        // find the boss spawn tile — boss starts hidden and gets enabled when stage reaches BOSS
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
            // tutorial only spawns normals, no elites — keep it manageable for new players
            Enemy* e = SpawnWeightedEnemy(pos, 1.f, 0.f);
            if (!e) continue;
            csvEnemies.push_back(e);
            std::cout << "[Tutorial] Spawned " << e->GetDefinition().name
                << " at (" << pos.x << ", " << pos.y << ")\n";
        }
        std::cout << "[Tutorial] Spawned " << csvEnemies.size() << " enemies total.\n";
        fairy->InitTutorial(gPlayer, *map, currentLevel);
        fairy->tilemap = map; // give fairy access to tilemap for door detection
        return;
    }

    // ── NORMAL MODE ───────────────────────────────────────────
    // CSV enemies first, then proc rooms after walking through connector
    totalKillTarget = 14 + rand() % 7;
    totalEnemiesRequired = totalKillTarget;
    std::cout << "[InitState] Kill target: " << totalKillTarget << "\n";

    std::vector<AEVec2> csvSpawnPool = FindSafeSpawnPositions(*map, 0, true);
    csvEnemies.clear();

    std::cout << "[InitState] csvSpawnPool has " << csvSpawnPool.size() << " positions.\n";
    gPlayer->SetHeldWeapon(0);
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

    // when player dies, alert game state so the end screen shows
    gPlayer->SubToOnDeath(this);
}

// =============================================================
void GameState::Update(double dt)
{
    GameEnd::Update(dt);
    if (GameEnd::IsOpen()) return;

    // M or ESC opens the pause menu (but not while the loading screen is up)
    if (loadingTimer <= 0.f &&
        (AEInputCheckTriggered(AEVK_M) || AEInputCheckTriggered(AEVK_ESCAPE)))
    {
        Pause::Toggle();
        return;
    }

    // Pause zeroes dt and handles its own input; skip all game logic while open
    Pause::Update(dt);
    if (Pause::IsOpen()) return;

    // sit here doing nothing until the loading timer runs out 
    // (ps to prof this loading screen is artifical to let our gamestate load and prevent the user from seeing the lagspike on load)
    if (loadingTimer > 0.f) {
        loadingTimer -= (float)dt;
        if (loadingTimer < 0.f) loadingTimer = 0.f;
        return;
    }

    // --- DEBUG INPUTS ---
#ifdef _DEBUG //only exists in debug
    if (AEInputCheckTriggered(AEVK_TAB)) {
        debugMode = !debugMode;
        showDebugOverlay = debugMode; // overlay follows debug mode
        if (!debugMode) {
            // reset all debug state when turning off
            showDebugOverlay = false;
            debugGodMode = false;
            debugFreezeEnemies = false;
            debugShowChests = false;
        }
        std::cout << "[Debug] Mode " << (debugMode ? "ON" : "OFF") << "\n";
    }

    // all debug keys below are only active when debug mode is on
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
        // spawn chest at cursor
        if (AEInputCheckTriggered(AEVK_L)) {
            LootChest* chest = dynamic_cast<LootChest*>(
                GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
            AEVec2 m = GetMouseWorldVec();
            chest->Init(m, { 75,75 }, 1, MESH_SQUARE, Collision::COL_RECT, { 75,75 },
                CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE);
        }
        //spawn enemy at cursor
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
    }
#endif

    // --- NON-DEBUG INPUTS ---
    // keyboard toggle — X shows/hides the keybind image popup
    if (AEInputCheckTriggered(AEVK_X)) {
        showKeyboardMenu = !showKeyboardMenu;
        std::cout << "[Menu] Keyboard " << (showKeyboardMenu ? "ON" : "OFF") << "\n";
    }

    // block all other input while the keyboard popup is showing
    if (showKeyboardMenu) return;

    // Cast pet skill (Standard gameplay, not debug)
    if (AEInputCheckTriggered(AEVK_R))
        PostOffice::GetInstance()->Send("PetManager", new PetSkillMsg(PetSkillMsg::CAST_SKILL));

    if (!gPlayer) return;

    if (debugGodMode)
        gPlayer->Heal(gPlayer->GetMaxHP());

    if (teleportCooldown > 0.f) teleportCooldown -= (float)dt;

    // tick the wave timer — wave 0 already fired on room entry, this handles subsequent waves
    if (inProceduralMap && !bossSpawned && nextMap) {
        procWaveTimer += (float)dt;
        if (procWaveTimer >= PROC_WAVE_INTERVAL) {
            procWaveTimer = 0.0f;
            SpawnProcWave(*nextMap);
        }
    }

    // tick the chest wave timer — one extra chest every 15 seconds while grinding
    if (inProceduralMap && !bossSpawned && nextMap) {
        procChestWaveTimer += (float)dt;
        if (procChestWaveTimer >= CHEST_WAVE_INTERVAL) {
            procChestWaveTimer = 0.0f;
            SpawnProcChestWave(*nextMap);
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
            // clear CSV enemies and any leftover drops so they don't bleed into the proc map
            DisableAndClearEnemies(csvEnemies);
            DisableAndClearEnemies(procEnemies);
            DropSystem::ClearAllPickups();
            DisableAndClearChests();
            procWaveNumber = 0;
            SpawnProcWave(*nextMap);
            procWaveTimer = 0.0f;
            procChestWaveNumber = 0;
            SpawnProcChestWave(*nextMap, 4); // drop 4 chests as soon as room load
            procChestWaveTimer = 0.0f;
            minimap->Reset();
            PetManager::GetInstance()->SetTilemap(*nextMap);
        }
        else if (inProceduralMap && nextMap->IsConnector(gPlayer->GetPos())) {
            nextMap->GenerateProcedural(50, 50, rand());
            AEVec2 procSpawn = nextMap->GetSpawnPoint();
            gPlayer->SetPos(procSpawn);
            gPlayer->Move({ 0,0 });
            PetManager::GetInstance()->SetTilemap(*nextMap);
            PetManager::GetInstance()->PlacePet(gPlayer->GetPos());
            camPos = procSpawn; camVel = { 0, 0 };
            halfMapWidth = nextMap->GetFullMapSize().x * 0.5f;
            halfMapHeight = nextMap->GetFullMapSize().y * 0.5f;
            SetCameraPos(camPos);
            teleportCooldown = 2.f;
            previousRoomsKilled = totalEnemiesKilled;
            // clear previous room's enemies and drops before the new ones spawn
            DisableAndClearEnemies(procEnemies);
            DropSystem::ClearAllPickups();
            DisableAndClearChests();
            procWaveNumber = 0;
            SpawnProcWave(*nextMap);
            procWaveTimer = 0.0f;
            procChestWaveNumber = 0;
            SpawnProcChestWave(*nextMap, 4); // drop 4 chests as soon as room load
            procChestWaveTimer = 0.0f;
            minimap->Reset();
        }
    }

    TileMap* currentMap = inProceduralMap ? nextMap : map;

    AEVec2 move = gPlayer->GetMoveDirNorm();
    f32 len = AEVec2Length(&move);
    if (len > 0 || gPlayer->HasForceApplied())
        playerDir = gPlayer->GetMoveDirNorm();

    // count dead enemies every frame to keep the progress bar accurate
    if (!(debugFreezeEnemies))
        CountAllDeadEnemies();

    if (inProceduralMap)
        TrySpawnBoss(*nextMap);

    if (boss && bossSpawned) {
        bossHPProgressBar = (boss->GetHP() / boss->GetMaxHP()) * bossMaxHPProgressBar;
        bossAlive = !boss->IsDead();
    }

    // tutorial boss is hidden until the fairy reaches the BOSS stage,
    // then it gets enabled and we watch for it to die
    if (doTutorial && fairy->data.stage == Tutorial::BOSS) {
        if (boss && !boss->IsDead()) {
            boss->SetEnabled(true);
            bossAlive = true;
        }
        if (boss && (boss->IsDead() || boss->GetHP() <= 0.f)) {
            bossAlive = false;
            boss->SetEnabled(false);
            fairy->ChangeStage(Tutorial::END);
        }
    }

    // tick the endless survival clock and notify the player each new minute
    if (endlessTimerActive) {
        endlessRunTimer += (float)dt;
        int currentMinute = (int)(endlessRunTimer / 60.0f);
        if (currentMinute > mLastMinuteMark) {
            mLastMinuteMark = currentMinute;
            mNotificationTimer = 5.0f; // show the "enemies are X% stronger" flash for 5 seconds
        }
    }

    if (mNotificationTimer > 0.0f) {
        mNotificationTimer -= (float)dt;
    }

    // death check before UpdateObjects so any heal-on-death doesn't undo it
    if (gPlayer && gPlayer->GetHP() <= 0.f && !GameEnd::IsOpen()) {
        if (mapSelected == "Assets/Endless.csv") {
            GameEnd::Show(false, true, endlessRunTimer, gPlayer->GetInventory().GetCoins(), totalEnemiesKilled);
            std::cout << "PLAYER DIED — Endless over.\n";
        }
        else {
            GameEnd::Show(false, false, 0.f, gPlayer->GetInventory().GetCoins(), totalEnemiesKilled);
            std::cout << "PLAYER DIED.\n";
        }
        return; // stop all further update logic this frame
    }

    // ── BOSS DEFEATED ─────────────────────────────────────────────────────────
    if (!doTutorial && bossSpawned && !bossAlive && !GameEnd::IsOpen()) {
        if (mapSelected != "Assets/Endless.csv") {
            // normal mode — show the win screen
            GameEnd::Show(true, false, 0.f, gPlayer->GetInventory().GetCoins(), totalEnemiesKilled);
            std::cout << "BOSS SLAYED\n";
            return;
        }
        else {
            // endless mode — start the 3-second countdown before resetting the room
            // player stays frozen during this window so they don't spawn inside a wall
            std::cout << "BOSS SLAYED — Endless countdown started!\n";
            bossRespawnPending = true;
            bossRespawnTimer = BOSS_RESPAWN_DELAY;
            bossSpawned = false;
            bossAlive = true;
            boss = nullptr;
            bossHPProgressBar = 0.f;
            bossMaxHPProgressBar = 100.f;
            DisableAndClearEnemies(procEnemies);
        }
    }

    // ── ENDLESS RESPAWN COUNTDOWN ──────────────────────────────────────────────
    // tick down after boss dies — actual map reset fires when timer hits zero
    if (bossRespawnPending) {
        bossRespawnTimer -= (float)dt;
        if (bossRespawnTimer <= 0.f) {
            // timer expired — reset kill tracking and generate fresh room
            bossRespawnPending = false;
            totalKillTarget = 20 + rand() % 31;
            totalEnemiesRequired = totalKillTarget;
            previousRoomsKilled = 0;
            totalEnemiesKilled = 0;
            enemiesKilledInRoom = 0;
            DropSystem::ClearAllPickups();
            DisableAndClearChests();
            std::cout << "[Endless] New kill target: " << totalKillTarget << "\n";
            teleportCooldown = 2.f;
            if (nextMap) {
                nextMap->GenerateProcedural(50, 50, rand());
                AEVec2 procSpawn = nextMap->GetSpawnPoint();
                // teleport player to the fresh spawn point before enemies start spawning
                gPlayer->SetPos(procSpawn);
                camPos = procSpawn; camVel = { 0, 0 };
                SetCameraPos(camPos);
                procWaveNumber = 0;
                SpawnProcWave(*nextMap);
                procWaveTimer = 0.0f;
                // drop 4 chests on the new room entry
                procChestWaveNumber = 0;
                SpawnProcChestWave(*nextMap, 4);
                procChestWaveTimer = 0.0f;
                minimap->Reset();
                PetManager::GetInstance()->SetTilemap(*nextMap);
            }
        }
        else {
            minimap->Update(dt, *currentMap, *gPlayer);
            UpdateWorldMap((float)dt);
            GameObjectManager::GetInstance()->UpdateObjects(dt, currentMap);
            DropSystem::UpdatePickupDisplay(static_cast<float>(dt));
            return;
        }
    }

    minimap->Update(dt, *currentMap, *gPlayer);
    UpdateWorldMap((float)dt);

    if (debugFreezeEnemies) {
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

    if (screenFlashTimer > 0.0f) screenFlashTimer -= (float)dt;

    DropSystem::UpdatePickupDisplay(static_cast<float>(dt));
}

// =============================================================
void GameState::Draw()
{
    // loading screen — black background with a progress bar while assets settle
    if (loadingTimer > 0.f) {
        float winW = (float)(AEGfxGetWinMaxX() - AEGfxGetWinMinX());
        float winH = (float)(AEGfxGetWinMaxY() - AEGfxGetWinMinY());

        AEMtx33 bgMtx;
        GetTransformMtx(bgMtx, { 0, 0 }, 0, { winW, winH });
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);
        AEGfxSetTransform(bgMtx.m);
        AEGfxSetColorToMultiply(0.f, 0.f, 0.f, 1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

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

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        if (font >= 0)
            AEGfxPrint(font, "LOADING...", -0.2f, 0.05f, 1.2f, 1.f, 1.f, 1.f, 1.f);

        return; // skip all other rendering while loading
    }

    RenderWorldMap();
    GameObjectManager::GetInstance()->DrawObjects();
    DrawEnemyStats(MakeDebugCtx());
    DrawBossHPProgressBar();
    TileMap* currentMap = inProceduralMap ? nextMap : map;
    minimap->Render(*currentMap, *gPlayer);
    DebugContext dbg = MakeDebugCtx();

    // chest dots on minimap — only visible when F7 is toggled in debug
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
    if (showKeybindOverlay) DrawKeybindOverlay(dbg);

    if (gPlayer) DrawPlayerUI();
    DrawHoveredItemStats();

    // [X] hint in bottom-right so players know the keybind popup exists
    if (!doTutorial && font >= 0) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxPrint(font, "[X] Keybinds", 0.62f, -0.88f, 0.5f, 1.f, 1.f, 1.f, 0.7f);
    }

    // endless survival timer at the top center, plus the difficulty notification flash
    if (endlessTimerActive && font >= 0) {
        int minutes = (int)endlessRunTimer / 60;
        int seconds = (int)endlessRunTimer % 60;
        char timeText[64];
        snprintf(timeText, sizeof(timeText), "SURVIVED: %02d:%02d", minutes, seconds);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxPrint(font, timeText, -0.4f, 0.75f, 1.0f, 1.f, 1.f, 1.f, 1.f);
        if (mNotificationTimer > 0.0f) {
            char flashText[128];
            int totalBuff = mLastMinuteMark * 10;
            snprintf(flashText, sizeof(flashText), "WARNING: ENEMIES ARE %d%% STRONGER!", totalBuff);
            float alpha = mNotificationTimer / 5.0f;
            AEGfxPrint(font, flashText, -0.80f, 0.5f, 1.2f, 1.0f, 0.2f, 0.2f, alpha);
        }
    }

    // --- ENDLESS RESPAWN COUNTDOWN TEXT ---
    if (bossRespawnPending && font >= 0) {
        int displaySecs = (int)std::ceilf(bossRespawnTimer);
        char cdText[64];
        snprintf(cdText, sizeof(cdText), "NEXT ROOM IN: %d", displaySecs);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxPrint(font, cdText, -0.28f, 0.50f, 1.2f, 1.0f, 0.0f, 0.0f, 1.0f);
    }

    HandleTutorialDialogueRender();
    PetManager::GetInstance()->DrawUI();
    DropSystem::PrintPickupDisplay();

    // brief red flash when the player takes a hit
    if (screenFlashTimer > 0.0f) {
        float winW = (float)AEGfxGetWinMaxX() - (float)AEGfxGetWinMinX();
        float winH = (float)AEGfxGetWinMaxY() - (float)AEGfxGetWinMinY();
        AEMtx33 mtx;
        GetTransformMtx(mtx, { 0, 0 }, 0, { winW, winH });
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);
        AEGfxSetTransform(mtx.m);
        float alpha = (screenFlashTimer / maxFlashDuration) * 0.3f; // max 30% opacity
        AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, alpha);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    DrawUIHoveredGearTooltip(invGearHoverInd);

    // keyboard popup — full-screen overlay showing all keybinds as an image
    if (showKeyboardMenu && keyboardTex) {
        float winW = (float)(AEGfxGetWinMaxX() - AEGfxGetWinMinX());
        float winH = (float)(AEGfxGetWinMaxY() - AEGfxGetWinMinY());

        float drawH = winH * 0.8f;
        float drawW = winW * 0.6f;
        AEMtx33 trans;
        GetTransformMtx(trans, { 0, 0 }, 0, { drawW, drawH });

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxTextureSet(keyboardTex, 0, 0);
        AEGfxSetTransform(trans.m);
        AEGfxSetTransparency(1.f);
        AEGfxSetColorToMultiply(1.f, 1.f, 1.f, 1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    // pause and end screens sit on top of everything else
    Pause::Draw();
    GameEnd::Draw();
}

void GameState::TriggerScreenFlash(float duration) {
    screenFlashTimer = maxFlashDuration = duration;
}

void GameState::HandleTutorialDialogueRender()
{
    if (!doTutorial || !fairy || !fairy->data.playDialogue) return;
    DrawAETextbox(font, fairy->data.dialogueLines[fairy->data.currDialogueLine].c_str(),
        fairy->data.dialoguePos, AEGfxGetWindowWidth() * 0.9f, fairy->data.dialogueSize, 0.0f,
        Color{ 0,0,0,255 }, TEXT_MIDDLE, TextboxOriginPos::TOP,
        TextboxBgCfg{ AEVec2{0.005f, 0.025f}, Color{255,255,255,255}, 255, RenderingManager::GetInstance()->GetMesh(MESH_SQUARE), nullptr });
}

// subscribed to player death events — shows the appropriate end screen
void GameState::SubscriptionAlert(ActorDeadSubContent content)
{
    if (content.victim != gPlayer) return;
    GameEnd::Show(false, mapSelected == "Assets/Endless.csv", endlessRunTimer, gPlayer->GetInventory().GetCoins(), totalEnemiesKilled);
}

// =============================================================
void GameState::ExitState()
{
    // save coins and endless high score before destroying anything
    if (gPlayer) {
        ShopFunctions::GetInstance()->addMoney(gPlayer->GetInventory().GetCoins());
        gPlayer->GetInventory().Clear();
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
    procChestWaveTimer = 0.0f;
    procChestWaveNumber = 0;
    bossHPProgressBar = 0.f;
    bossMaxHPProgressBar = 100.f;
    doTutorial = false;
    boss = nullptr;
    DisableAndClearEnemies(csvEnemies);
    DisableAndClearEnemies(procEnemies);
    DisableAndClearChests();
    DropSystem::ClearAllPickups();

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