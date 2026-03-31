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
    AEGfxVertexList* circleMesh = nullptr;
    AEGfxVertexList* wallMesh = nullptr;
    AEGfxVertexList* squareMesh = nullptr;

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

        //Not loaded from json
        AEVec2 hpBarPos{};
        AEMtx33 hpBarTrans{};
    } playerUISettings;

    std::array<UIElement*, 7> invGearElements{};
    int invGearHoverInd{-1};

    // --- BOSS / ENEMY TRACKING ---
    bool   bossAlive = true;
    bool   bossSpawned = false;
    float  bossRadius = 60.0f;
    Enemy* boss = nullptr;

    std::vector<Enemy*> csvEnemies;
    std::vector<Enemy*> procEnemies;

    int   enemiesKilledInRoom = 0;
    int   enemiesRequiredForBoss = 0;

    int   totalEnemiesKilled = 0;
    int   totalEnemiesRequired = 0;
    int   previousRoomsKilled = 0;
    int   totalKillTarget = 0;

    float bossSpawnThreshold = 1.0f;

    float procWaveTimer = 0.0f;
    int   procWaveNumber = 0;
    const float PROC_WAVE_INTERVAL = 4.0f;

    float procChestWaveTimer = 0.0f;
    int   procChestWaveNumber = 0;
    const float CHEST_WAVE_INTERVAL = 15.0f;

    AEVec2 camPos, camVel;
    float  camZoom = 1.2f;
    float  camSmoothTime = 0.15f;

    float    mapWidth = 2400.0f;
    float    mapHeight = 2000.0f;
    float    halfMapWidth, halfMapHeight;
    MapData  currentLevel;
    TileMap* map{};
    TileMap* nextMap{};
    Minimap* minimap{};
    float    teleportCooldown = 0.f;
    bool     inProceduralMap = false;

    bool doTutorial{ true };
    Tutorial::TutorialFairy* fairy;
    s8 font{ -1 };

    float bossHPProgressBarWidth = 0.f;
    float bossHPProgressBarHeight = 0.f;
    float bossHPProgressBar = 100.f;
    float bossMaxHPProgressBar = 100.f;

    bool debugMode = false;
    bool showDebugOverlay = false;
    bool showKeybindOverlay = false;
    bool debugGodMode = false;
    bool debugFreezeEnemies = false;
    bool debugShowChests = false;

    float loadingTimer = 0.f;
    const float LOADING_DURATION = 7.f;

    float endlessRunTimer = 0.f;
    bool endlessTimerActive = false;
    
    float screenFlashTimer = 0.0f;
    float maxFlashDuration = 0.5f;

    // keyboard popup
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

    std::vector<AEVec2> FindSafeSpawnPositions(TileMap const& tilemap, int maxCount, bool trustMarkers = true)
    {
        std::vector<AEVec2> positions;
        auto mapSz = tilemap.GetMapSize();
        unsigned cols = mapSz.first;
        unsigned rows = mapSz.second;

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

    void SpawnProcChestWave(TileMap const& tilemap, int count = 1)
    {
        std::vector<AEVec2> safePool = CollectProcSafePositions(tilemap,2);
        if (safePool.empty()) return;

        const float MIN_CHEST_SPACING = 115.f * 5.f;

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

                AEVec2 gridInd = tilemap.GetTileIndFromPos(pos);
                int r = (int)gridInd.y, c = (int)gridInd.x;
                bool strictlySafe = true;
                for (int dr = -2; dr <= 2 && strictlySafe; ++dr)
                    for (int dc = -2; dc <= 2 && strictlySafe; ++dc) {
                        const TileMap::Tile* n = tilemap.QueryTile(r + dr, c + dc);
                        if (!n || n->type != TileMap::TILE_NONE) strictlySafe = false;
                    }
                if (!strictlySafe) continue;

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

                placed.push_back(pos);
                ++procChestWaveNumber;
                ++spawned;
                found = true;
                std::cout << "[ChestWave " << procChestWaveNumber << "] Spawned chest at ("
                    << pos.x << ", " << pos.y << ")\n";
                break;
            }
            if (!found) break;
        }
        std::cout << "[ChestWave] Spawned " << spawned << " chest(s) this wave.\n";
    }

    void DisableAndClearEnemies(std::vector<Enemy*>& enemies)
    {
        for (Enemy* e : enemies)
            if (e && e->IsEnabled()) e->SetEnabled(false);
        enemies.clear();
    }

    void DisableAndClearChests()
    {
        auto& gos = GameObjectManager::GetInstance()->GetGameObjects();
        for (GameObject* go : gos) {
            if (go && go->IsEnabled() && go->GetGOType() == GO_TYPE::LOOT_CHEST)
                go->SetEnabled(false);
        }
        std::cout << "[ClearChests] All chests disabled.\n";
    }

    void SpawnProcWave(TileMap const& tilemap)
    {
        const int MAX_LIVE_ENEMIES = 50;
        const float MIN_SPAWN_SPACING = 32.f * 3.f; // ~3 tiles gap between spawns

        // 5% increase every 60 seconds)
        float currentDifficulty = 1.0f;
        if (mapSelected == "Assets/Endless.csv") {
            currentDifficulty = 1.0f + (endlessRunTimer / 60.0f) * 0.05f;
        }
        int liveCount = 0;
        std::vector<AEVec2> chosen;
        for (Enemy* e : procEnemies) {
            if (e && e->IsEnabled() && e->GetHP() > 0.f) {
                ++liveCount;
                chosen.push_back(e->GetPos());
            }
        }

        int canSpawn = MAX_LIVE_ENEMIES - liveCount;
        if (canSpawn <= 0) return;

        int waveSize = (procWaveNumber == 0) ? 12 : (4 + procWaveNumber);
        waveSize = (waveSize < canSpawn) ? waveSize : canSpawn;
        ++procWaveNumber;

        std::vector<AEVec2> safePool = CollectProcSafePositions(tilemap, 1);
        if (safePool.empty()) return;

        int spawned = 0;
        for (int i = 0; i < waveSize && !safePool.empty(); ++i) {
            bool placed = false;
            for (int attempt = 0; attempt < 20 && !safePool.empty(); ++attempt) {
                int idx = rand() % (int)safePool.size();
                AEVec2 pos = safePool[idx];

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
            if (!placed) continue;
        }
    }

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

        DisableAndClearEnemies(procEnemies);
        DisableAndClearEnemies(csvEnemies);

        std::cout << "[Boss] Spawned " << boss->GetDefinition().name
            << "! (kill target was " << totalKillTarget << ")\n";
    }

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
        if (!bossAlive) {
            DrawTintedMesh(
                GetTransformMtx({ currentLevel.doorPos.x, currentLevel.doorPos.y },
                    0, { 80, 80 }),
                wallMesh, nullptr, { 0, 0.8f * 255, 0, 255 }, 255);
        }
        if (inProceduralMap) { if (nextMap) nextMap->Render(); }
        else { map->Render(); }
    }

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

    void DrawPlayerUI() {
        int fontSz = RenderingManager::GetInstance()->GetFontSize();
        if (gPlayer->ShowStatsUI()) {
            Inventory const& mInventory{ gPlayer->GetInventory() };
            AEVec2 bgPos = { -700.0f, 45.0f };
            AEVec2 bgSize = { 600.0f, 520.0f };
            DrawTintedMesh(GetTransformMtx(bgPos, 0.0f, bgSize), squareMesh, nullptr, { 0, 0, 0, 180 }, 255);

            std::string coinText = "Coins: " + std::to_string(mInventory.GetCoins());
            DrawAEText(font, coinText.c_str(), { -780.0f, 280.0f }, 0.5f, { 255, 215, 0, 255 }, TEXT_MIDDLE_LEFT);
            std::string ammoText = "Ammo: " + std::to_string(mInventory.GetAmmo());
            DrawAEText(font, ammoText.c_str(), { -780.0f,  250.0f }, 0.5f, { 200, 200, 200, 255 }, TEXT_MIDDLE_LEFT);

            AEVec2 textPos = { -780.0f, 200.0f };
            float yLineSpc = -20.0f;

            ActorStats const& mStats{ gPlayer->GetStats() };
            DrawAEText(font, "--- STATS ---", textPos, 0.4f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
            DrawAEText(font, ("Max HP: " + std::to_string((int)mStats.maxHP)).c_str(), textPos, 0.4f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
            DrawAEText(font, ("Attack: " + std::to_string((int)mStats.attack)).c_str(), textPos, 0.4f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
            DrawAEText(font, ("Defense: " + std::to_string((int)mStats.defense)).c_str(), textPos, 0.4f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;
            DrawAEText(font, ("Move Speed: " + std::to_string((int)mStats.moveSpeed)).c_str(), textPos, 0.4f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;

            std::string tSpd = std::to_string(mStats.attackSpeed);
            size_t spdLen = tSpd.length() > 4 ? 4 : tSpd.length();
            DrawAEText(font, ("Atk Speed: " + tSpd.substr(0, spdLen)).c_str(), textPos, 0.4f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc * 2.0f;

            DrawAEText(font, "--- EQUIPMENT ---", textPos, 0.4f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y += yLineSpc;

            auto w1 = mInventory.GetMainWeapon(0);
            auto w2 = mInventory.GetMainWeapon(1);
            auto bow = mInventory.GetBow();
            auto head = mInventory.GetArmor(ArmorSlot::Head);
            auto body = mInventory.GetArmor(ArmorSlot::Body);
            auto hands = mInventory.GetArmor(ArmorSlot::Hands);
            auto feet = mInventory.GetArmor(ArmorSlot::Feet);

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
    void DrawUIHoveredGearTooltip(int ind) {
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
        if (!eq) return;

        //Draw tooltip
        float winW = (float)AEGfxGetWinMaxX() - AEGfxGetWinMinX();
        float winH = (float)AEGfxGetWinMaxY() - AEGfxGetWinMinY();
        AEVec2 bgSize = { 300.0f, 220.0f };
        AEVec2 bgPos = { mP.x + bgSize.x * 0.5f + 15.0f, mP.y - bgSize.y * 0.5f - 15.0f };

        if (bgPos.x + bgSize.x * 0.5f > winW * 0.5f) bgPos.x = winW * 0.5f - bgSize.x * 0.5f;
        if (bgPos.y - bgSize.y * 0.5f < -winH * 0.5f) bgPos.y = -winH * 0.5f + bgSize.y * 0.5f;

        DrawTintedMesh(GetTransformMtx(bgPos, 0.0f, bgSize), squareMesh, nullptr, { 0, 0, 0, 220 }, 255);

        AEVec2 textPos = { bgPos.x - bgSize.x * 0.5f + 15.0f, bgPos.y + bgSize.y * 0.5f - 25.0f };
        Color rColor = GetRarityColor(eq->rarity);

        DrawAEText(font, eq->name, textPos, 0.45f, rColor, TEXT_MIDDLE_LEFT);
        textPos.y -= 30.0f;

        DrawAEText(font, ("Sell Price: " + std::to_string(eq->sellPrice)).c_str(), textPos, 0.35f, { 255, 215, 0, 255 }, TEXT_MIDDLE_LEFT);
        textPos.y -= 35.0f;

        if (eq->mods.additive.maxHP != 0) { DrawAEText(font, ("Max HP: +" + std::to_string((int)eq->mods.additive.maxHP)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
        if (eq->mods.additive.attack != 0) { DrawAEText(font, ("Attack: +" + std::to_string((int)eq->mods.additive.attack)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
        if (eq->mods.additive.defense != 0) { DrawAEText(font, ("Defense: +" + std::to_string((int)eq->mods.additive.defense)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
        if (eq->mods.additive.moveSpeed != 0) { DrawAEText(font, ("Speed: +" + std::to_string((int)eq->mods.additive.moveSpeed)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
        if (eq->mods.additive.attackSpeed != 0) {
            std::string aSpdStr = std::to_string(eq->mods.additive.attackSpeed);
            aSpdStr = aSpdStr.substr(0, aSpdStr.find('.') + 3);
            DrawAEText(font, ("Atk Speed: +" + aSpdStr).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f;
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

                    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                    AEGfxTextureSet(nullptr, 0, 0);

                    s32 mX, mY;
                    AEInputGetCursorPosition(&mX, &mY);
                    
                    float winW = (float)AEGfxGetWinMaxX() - AEGfxGetWinMinX();
                    float winH = (float)AEGfxGetWinMaxY() - AEGfxGetWinMinY();
                    float screenX = (float)mX - (winW * 0.5f);
                    float screenY = (winH * 0.5f) - (float)mY;

                    AEVec2 bgSize = { 300.0f, 220.0f };
                    AEVec2 bgPos = { screenX + bgSize.x * 0.5f + 15.0f, screenY - bgSize.y * 0.5f - 15.0f };
                    
                    if (bgPos.x + bgSize.x * 0.5f > winW * 0.5f) bgPos.x = winW * 0.5f - bgSize.x * 0.5f;
                    if (bgPos.y - bgSize.y * 0.5f < -winH * 0.5f) bgPos.y = -winH * 0.5f + bgSize.y * 0.5f;

                    DrawTintedMesh(GetTransformMtx(bgPos, 0.0f, bgSize), squareMesh, nullptr, { 0, 0, 0, 220 }, 255);

                    AEVec2 textPos = { bgPos.x - bgSize.x * 0.5f + 15.0f, bgPos.y + bgSize.y * 0.5f - 25.0f };
                    Color rColor = GetRarityColor(eq->rarity);
                    
                    DrawAEText(font, eq->name, textPos, 0.45f, rColor, TEXT_MIDDLE_LEFT);
                    textPos.y -= 30.0f;
                    
                    DrawAEText(font, ("Sell Price: " + std::to_string(eq->sellPrice)).c_str(), textPos, 0.35f, { 255, 215, 0, 255 }, TEXT_MIDDLE_LEFT);
                    textPos.y -= 35.0f;
                    
                    if (eq->mods.additive.maxHP != 0) { DrawAEText(font, ("Max HP: +" + std::to_string((int)eq->mods.additive.maxHP)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
                    if (eq->mods.additive.attack != 0) { DrawAEText(font, ("Attack: +" + std::to_string((int)eq->mods.additive.attack)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
                    if (eq->mods.additive.defense != 0) { DrawAEText(font, ("Defense: +" + std::to_string((int)eq->mods.additive.defense)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
                    if (eq->mods.additive.moveSpeed != 0) { DrawAEText(font, ("Speed: +" + std::to_string((int)eq->mods.additive.moveSpeed)).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; }
                    if (eq->mods.additive.attackSpeed != 0) { 
                        std::string aSpdStr = std::to_string(eq->mods.additive.attackSpeed);
                        aSpdStr = aSpdStr.substr(0, aSpdStr.find('.') + 3);
                        DrawAEText(font, ("Atk Speed: +" + aSpdStr).c_str(), textPos, 0.35f, { 255, 255, 255, 255 }, TEXT_MIDDLE_LEFT); textPos.y -= 25.0f; 
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

    if (map) { delete map; map = nullptr; }
    if (mapSelected == "Assets/Endless.csv") {
        map = new TileMap({ 0.f, 0.f }, 115.f, 115.f);
    }
    else {
        map = new TileMap(mapSelected, { 0,0 }, 115.f, 115.f);
        std::cout << "[InitState] Reloaded map: " << mapSelected << "\n";
    }

    mapWidth = map->GetFullMapSize().x + nextMap->GetFullMapSize().x;
    mapHeight = (map->GetFullMapSize().y > nextMap->GetFullMapSize().y)
        ? map->GetFullMapSize().y : nextMap->GetFullMapSize().y;
    halfMapWidth = mapWidth * 0.5f;
    halfMapHeight = mapHeight * 0.5f;

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

    //Load Inventory UI variables
    playerUISettings.invSize ={ 2 * (playerUISettings.invEdgePadding.x + playerUISettings.invGearIconSz.x) + playerUISettings.invIconGap,
                7 * (RenderingManager::GetInstance()->GetFontSize() * playerUISettings.invStatFontSz) + 6*playerUISettings.invStatGap
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
        invGearElements[i]->SetHoverCallback([i](bool) {invGearHoverInd = i;});
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

    if (mapSelected == "Assets/Endless.csv") {
        std::cout << "[InitState] Endless mode.\n";
        gPlayer->SetHeldWeapon(0);
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
        procWaveTimer = 0.0f;

        procChestWaveNumber = 0;
        SpawnProcChestWave(*nextMap, 3);
        procChestWaveTimer = 0.0f;

        loadingTimer = LOADING_DURATION;
        endlessRunTimer = 0.f;
        endlessTimerActive = true;

        //When player dies, alert game state
        gPlayer->SubToOnDeath(this);

        PetManager::GetInstance()->SetTilemap(*nextMap);
        minimap->Reset();
        return;
    }

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
    //Call pet stuff after player setup
    if (!doTutorial) {
        PetManager::GetInstance()->InitPetForGame(*map);
    }
    else {
        PetManager::GetInstance()->UnsetPet();
    }

    SpawnCsvChests(*map);

    camPos = safeSpawnPos;
    camVel = { 0, 0 };
    minimap->Reset();

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
                        boss->SetEnabled(false);
                    }
                    break;
                }
            }
        }

        std::vector<AEVec2> csvSpawnPool = FindSafeSpawnPositions(*map, 0, true);
        csvEnemies.clear();
        for (AEVec2 const& pos : csvSpawnPool) {
            Enemy* e = SpawnWeightedEnemy(pos, 1.f, 0.f);
            if (!e) continue;
            csvEnemies.push_back(e);
            std::cout << "[Tutorial] Spawned " << e->GetDefinition().name
                << " at (" << pos.x << ", " << pos.y << ")\n";
        }
        std::cout << "[Tutorial] Spawned " << csvEnemies.size() << " enemies total.\n";
        fairy->InitTutorial(gPlayer, *map, currentLevel);
        fairy->tilemap = map;
        return;
    }

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

    //When player dies, alert game state
    gPlayer->SubToOnDeath(this);
}

// =============================================================
void GameState::Update(double dt)
{
    GameEnd::Update(dt);
    if (GameEnd::IsOpen()) return;

    if (loadingTimer <= 0.f &&
        (AEInputCheckTriggered(AEVK_M) || AEInputCheckTriggered(AEVK_ESCAPE)))
    {
        Pause::Toggle();
        return;
    }

    if (Pause::Update()) return;

    if (loadingTimer > 0.f) {
        loadingTimer -= (float)dt;
        if (loadingTimer < 0.f) loadingTimer = 0.f;
        return;
    }

    if (AEInputCheckTriggered(AEVK_TAB)) {
        showDebugOverlay = !showDebugOverlay;
        std::cout << "[Debug] Overlay " << (showDebugOverlay ? "ON" : "OFF") << "\n";
    }

    if (AEInputCheckTriggered(AEVK_K))
        showKeybindOverlay = !showKeybindOverlay;

    if (AEInputCheckTriggered(AEVK_F5)) {
        debugGodMode = !debugGodMode;
        std::cout << "[Debug] God mode " << (debugGodMode ? "ON" : "OFF") << "\n";
    }

    // keyboard toggle
    if (AEInputCheckTriggered(AEVK_X)) {
        showKeyboardMenu = !showKeyboardMenu;
        std::cout << "[Menu] Keyboard " << (showKeyboardMenu ? "ON" : "OFF") << "\n";
    }

    if (showKeyboardMenu) return;
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

    //Cast pet skill
    if (AEInputCheckTriggered(AEVK_R))
        PostOffice::GetInstance()->Send("PetManager", new PetSkillMsg(PetSkillMsg::CAST_SKILL));

#pragma region inputs_for_testing
    if (AEInputCheckTriggered(AEVK_L)) {
        LootChest* chest = dynamic_cast<LootChest*>(
            GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
        AEVec2 m = GetMouseWorldVec();
        chest->Init(m, { 75,75 }, 1, MESH_SQUARE, Collision::COL_RECT, { 75,75 },
            CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE);
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
        }
    }
#pragma endregion

    if (!gPlayer) return;

    if (debugGodMode)
        gPlayer->Heal(gPlayer->GetMaxHP());

    if (teleportCooldown > 0.f) teleportCooldown -= (float)dt;

    if (inProceduralMap && !bossSpawned && nextMap) {
        procWaveTimer += (float)dt;
        if (procWaveTimer >= PROC_WAVE_INTERVAL) {
            procWaveTimer = 0.0f;
            SpawnProcWave(*nextMap);
        }
    }

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
            DisableAndClearEnemies(csvEnemies);
            DisableAndClearEnemies(procEnemies);
            DropSystem::ClearAllPickups();
            DisableAndClearChests();
            procWaveNumber = 0;
            SpawnProcWave(*nextMap);
            procWaveTimer = 0.0f;
            procChestWaveNumber = 0;
            SpawnProcChestWave(*nextMap, 4);
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
            DisableAndClearEnemies(procEnemies);
            DropSystem::ClearAllPickups();
            DisableAndClearChests();
            procWaveNumber = 0;
            SpawnProcWave(*nextMap);
            procWaveTimer = 0.0f;
            procChestWaveNumber = 0;
            SpawnProcChestWave(*nextMap, 4);
            procChestWaveTimer = 0.0f;
            minimap->Reset();
        }
    }

    TileMap* currentMap = inProceduralMap ? nextMap : map;

    AEVec2 move = gPlayer->GetMoveDirNorm();
    f32 len = AEVec2Length(&move);
    if (len > 0 || gPlayer->HasForceApplied())
        playerDir = gPlayer->GetMoveDirNorm();

    if (!(debugFreezeEnemies))
        CountAllDeadEnemies();

    if (inProceduralMap)
        TrySpawnBoss(*nextMap);

    if (boss && bossSpawned) {
        bossHPProgressBar = (boss->GetHP() / boss->GetMaxHP()) * bossMaxHPProgressBar;
        bossAlive = !boss->IsDead();
    }

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

    if (endlessTimerActive) {
        endlessRunTimer += (float)dt;
        int currentMinute = (int)(endlessRunTimer / 60.0f);
        if (currentMinute > mLastMinuteMark) {
            mLastMinuteMark = currentMinute;
            mNotificationTimer = 5.0f; 
        }
    }

    if (mNotificationTimer > 0.0f) {
        mNotificationTimer -= (float)dt;
    }
    
    // ── DEATH CHECK before UpdateObjects so heal in OnDeath doesn't undo it ──
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
            GameEnd::Show(true, false, 0.f, gPlayer->GetInventory().GetCoins(), totalEnemiesKilled);
            std::cout << "BOSS SLAYED\n";
            return;
        }
        else {
            std::cout << "BOSS SLAYED — Endless continues!\n";
            bossSpawned = false;
            bossAlive = true;
            boss = nullptr;
            bossHPProgressBar = 0.f;
            bossMaxHPProgressBar = 100.f;
            totalKillTarget = 20 + rand() % 31;
            totalEnemiesRequired = totalKillTarget;
            previousRoomsKilled = 0;
            totalEnemiesKilled = 0;
            enemiesKilledInRoom = 0;
            DisableAndClearEnemies(procEnemies);
            DropSystem::ClearAllPickups();
            DisableAndClearChests();
            std::cout << "[Endless] New kill target: " << totalKillTarget << "\n";
            teleportCooldown = 2.f;
            if (nextMap) {
                nextMap->GenerateProcedural(50, 50, rand());
                procWaveNumber = 0;
                SpawnProcWave(*nextMap);
                procWaveTimer = 0.0f;
                procChestWaveNumber = 0;
                SpawnProcChestWave(*nextMap, 4);
                procChestWaveTimer = 0.0f;
                minimap->Reset();
                PetManager::GetInstance()->SetTilemap(*nextMap);
            }
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
            AEGfxPrint(font, "LOADING...", -0.12f, 0.05f, 1.2f, 1.f, 1.f, 1.f, 1.f);

        return;
    }

    RenderWorldMap();
    GameObjectManager::GetInstance()->DrawObjects();
    DrawEnemyStats(MakeDebugCtx());
    DrawBossHPProgressBar();
    TileMap* currentMap = inProceduralMap ? nextMap : map;
    minimap->Render(*currentMap, *gPlayer);
    DebugContext dbg = MakeDebugCtx();

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

    // [X] Keybinds hint in bottom-right corner (non-tutorial only)
    if (!doTutorial && font >= 0) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxPrint(font, "[X] Keybinds", 0.62f, -0.88f, 0.5f, 1.f, 1.f, 1.f, 0.7f);
    }

    if (endlessTimerActive && font >= 0) {
        int minutes = (int)endlessRunTimer / 60;
        int seconds = (int)endlessRunTimer % 60;
        char timeText[64];
        snprintf(timeText, sizeof(timeText), "SURVIVED: %02d:%02d", minutes, seconds);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxPrint(font, timeText, -0.4f, 0.75f, 1.0f, 1.f, 1.f, 1.f, 1.f);
        if (mNotificationTimer > 0.0f) {
            char flashText[128];
            int totalBuff = mLastMinuteMark * 5;
            snprintf(flashText, sizeof(flashText), "WARNING: ENEMIES ARE %d%% STRONGER!", totalBuff);
            float alpha = mNotificationTimer / 5.0f;
            AEGfxPrint(font, flashText, -0.80f, 0.5f, 1.2f, 1.0f, 0.2f, 0.2f, alpha);
        }
    }

    HandleTutorialDialogueRender();
    PetManager::GetInstance()->DrawUI();
    DropSystem::PrintPickupDisplay();

    if (screenFlashTimer > 0.0f) {
        float winW = (float)AEGfxGetWinMaxX() - (float)AEGfxGetWinMinX();
        float winH = (float)AEGfxGetWinMaxY() - (float)AEGfxGetWinMinY();
        AEMtx33 mtx;
        GetTransformMtx(mtx, { 0, 0 }, 0, { winW, winH });
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);
        AEGfxSetTransform(mtx.m);
        float alpha = (screenFlashTimer / maxFlashDuration) * 0.3f; // Max 30% alpha
        AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, alpha);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    DrawUIHoveredGearTooltip(invGearHoverInd);

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

void GameState::SubscriptionAlert(ActorDeadSubContent content)
{
    if (content.victim != gPlayer) return;
    //Player died, end
    GameEnd::Show(false, mapSelected == "Assets/Endless.csv", endlessRunTimer, gPlayer->GetInventory().GetCoins(), totalEnemiesKilled);
}

// =============================================================
void GameState::ExitState()
{
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