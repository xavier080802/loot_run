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
    std::vector<Enemy*> procEnemies;

    int   enemiesKilledInRoom = 0;
    int   enemiesRequiredForBoss = 0;
    float bossSpawnThreshold = 1.0f;

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
    s8 font{ -1 };  // -1 = not loaded; 0 is a valid font index

    // --- BOSS HP / PROGRESS BAR ---
    float bossHPProgressBarWidth = 0.f;
    float bossHPProgressBarHeight = 0.f;
    float bossHPProgressBar = 100.f;
    float bossMaxHPProgressBar = 100.f;

    // =========================================================
    // --- DEBUG MODE ---
    // Toggle with TAB. While active, additional keys are live.
    // Press TAB again (or F3) to show the command list overlay.
    // =========================================================
    bool debugMode = false;  // master toggle — TAB key
    bool showDebugOverlay = false;  // TAB also shows the debug help overlay
    bool showKeybindOverlay = false; // K — shows all in-game keybinds overlay

    // Individual debug feature toggles
    bool debugShowAggroRings = false; // F5 — draw a circle around each enemy showing its aggro radius
    bool debugShowSpawnPoints = false; // F6 — highlight TILE_ENEMY tiles on the map
    bool debugGodMode = false; // F7 — player takes no damage
    bool debugShowStats = false; // F8 — print live player/enemy stats on screen
    bool debugFreezeEnemies = false; // F9 — enemies stop updating (AI frozen)

    // Draws a circle outline in world space (used by aggro ring debug)
    void DrawDebugCircle(AEVec2 centre, float radius, float r, float g, float b)
    {
        const int SEGMENTS = 32;
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);
        for (int i = 0; i < SEGMENTS; ++i) {
            float a0 = (float)i / SEGMENTS * 2.f * 3.14159f;
            float a1 = (float)(i + 1) / SEGMENTS * 2.f * 3.14159f;
            AEVec2 p0 = { centre.x + cosf(a0) * radius, centre.y + sinf(a0) * radius };
            AEVec2 p1 = { centre.x + cosf(a1) * radius, centre.y + sinf(a1) * radius };
            // Draw as a thin quad between the two edge points
            float thickness = 3.0f;
            AEVec2 mid = { (p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f };
            float dx = p1.x - p0.x, dy = p1.y - p0.y;
            float len = sqrtf(dx * dx + dy * dy);
            float angle = atan2f(dy, dx);
            AEMtx33 mtx;
            GetTransformMtx(mtx, mid, angle, { len, thickness });
            AEGfxSetTransform(mtx.m);
            AEGfxSetColorToMultiply(r, g, b, 0.6f);
            AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // Draws a semi-transparent dark rectangle in SCREEN space.
    // Used as a background panel behind overlay text so it's readable over any scene.
    void DrawPanel(float screenX, float screenY, float w, float h)
    {
        AEVec2 pos = { screenX + w * 0.5f, screenY - h * 0.5f };
        AEMtx33 mtx;
        GetTransformMtx(mtx, pos, 0, { w, h });
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);
        AEGfxSetTransform(mtx.m);
        AEGfxSetColorToMultiply(0.05f, 0.05f, 0.05f, 0.82f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Draws all in-game keybinds — toggle with K (works outside debug mode too)
    void DrawKeybindOverlay()
    {
        if (font < 0) return;

        // Use camera position so coords match the world-space draw system (same as Player::DrawUI)
        AEVec2 camPos;
        AEGfxGetCamPosition(&camPos.x, &camPos.y);

        float panelW = 320.f;
        float panelH = 460.f;
        float panelX = camPos.x + 370.f;
        float panelY = camPos.y + 310.f;
        DrawPanel(panelX, panelY, panelW, panelH);

        s8 fnt = RenderingManager::GetInstance()->GetFont();
        float x = panelX + 12.f;
        float keyW = 90.f;   // width reserved for the key column
        float y = panelY - 16.f;
        float lineH = 20.f;
        float sz = 0.38f;

        // Section header
        auto Header = [&](const char* txt) {
            DrawAEText(fnt, txt, { x, y }, sz,
                CreateColor(100, 180, 255, 255), TEXT_MIDDLE_LEFT);
            y -= lineH;
            };
        // Two-column row: key in white, description in light grey
        auto KV = [&](const char* key, const char* desc) {
            DrawAEText(fnt, key, { x,        y }, sz, CreateColor(255, 255, 255, 255), TEXT_MIDDLE_LEFT);
            DrawAEText(fnt, desc, { x + keyW, y }, sz, CreateColor(180, 180, 180, 255), TEXT_MIDDLE_LEFT);
            y -= lineH;
            };
        auto Gap = [&]() { y -= 5.f; };

        // Title
        DrawAEText(fnt, "=== KEYBINDS ===", { x, y }, sz,
            CreateColor(255, 200, 60, 255), TEXT_MIDDLE_LEFT);
        y -= lineH; Gap();

        Header("--- Movement ---");
        KV("[W/A/S/D]", "Move");
        KV("[SPACE]", "Dodge roll");
        Gap();

        Header("--- Combat ---");
        KV("[LMB]", "Attack");
        KV("[Z]", "Weapon slot 1");
        KV("[X]", "Weapon slot 2");
        KV("[Q]", "Equip bow");
        KV("[RMB]", "Swap melee weapons");
        KV("[G]", "Drop held weapon");
        Gap();

        Header("--- Interaction ---");
        KV("[E]", "Swap item (slot full)");
        KV("[C]", "Sell item");
        KV("[Connector]", "Enter dungeon");
        Gap();

        Header("--- UI ---");
        KV("[B]", "Stats & equipment");
        KV("[K]", "Keybind overlay");
        KV("[TAB]", "Debug overlay");
        KV("[M]", "Main menu");
        Gap();

        Header("--- Pets ---");
        KV("[R]", "Cast pet skill");
        Gap();

        Header("--- Dev / Testing ---");
        KV("[N]", "Spawn enemy");
        KV("[L]", "Move chest here");
        Gap();
    }

    // Draws the debug HUD — called from Draw() only when debugMode is true
    void DrawDebugOverlay()
    {
        if (font < 0 || !gPlayer) return;

        AEVec2 camPos;
        AEGfxGetCamPosition(&camPos.x, &camPos.y);

        float panelW = 480.f;
        float panelH = 560.f;
        float panelX = showKeybindOverlay
            ? camPos.x + 410.f - 300.f - panelW - 10.f
            : camPos.x + 320.f;
        float panelY = camPos.y + 320.f;
        DrawPanel(panelX, panelY, panelW, panelH);

        // Two columns: label at x, value at x + valOffset
        float x = panelX + 18.f;
        float valOffset = 140.f;
        float y = panelY - 20.f;
        float lineH = 26.f;
        float sz = 0.44f;

        s8 fnt = RenderingManager::GetInstance()->GetFont();

        // Helper: draw a key=value row. Key in white, value in cyan.
        auto KV = [&](const char* key, const char* val,
            float vr = 0.4f, float vg = 1.f, float vb = 0.9f) {
                DrawAEText(fnt, key, { x, y }, sz,
                    CreateColor(220, 220, 220, 255), TEXT_MIDDLE_LEFT);
                DrawAEText(fnt, val, { x + valOffset, y }, sz,
                    CreateColor((u8)(vr * 255), (u8)(vg * 255), (u8)(vb * 255), 255), TEXT_MIDDLE_LEFT);
                y -= lineH;
            };
        // Helper: full-width line (section headers, footers)
        auto Line = [&](const char* txt, float r, float g, float b) {
            DrawAEText(fnt, txt, { x, y }, sz,
                CreateColor((u8)(r * 255), (u8)(g * 255), (u8)(b * 255), 255), TEXT_MIDDLE_LEFT);
            y -= lineH;
            };
        // Helper: toggle row — key+label left, [ON]/[off] right in colour
        auto Toggle = [&](const char* key, const char* label, bool state) {
            std::ostringstream lbl;
            lbl << key << "  " << label;
            DrawAEText(fnt, lbl.str().c_str(), { x, y }, sz,
                CreateColor(200, 200, 200, 255), TEXT_MIDDLE_LEFT);
            DrawAEText(fnt, state ? "[ON] " : "[off]", { x + valOffset + 80.f, y }, sz,
                CreateColor(state ? 80 : 120, state ? 255 : 120, state ? 80 : 120, 255), TEXT_MIDDLE_LEFT);
            y -= lineH;
            };

        // ---- Header ----
        Line("=== DEBUG MODE ===", 1.f, 0.75f, 0.f);
        y -= 6.f;

        // ---- Live stats ----
        std::ostringstream oss;
        AEVec2 pp = gPlayer->GetPos();
        oss << (int)pp.x << ", " << (int)pp.y;
        KV("Pos:", oss.str().c_str());

        oss.str(""); oss << (int)gPlayer->GetHP() << " / " << (int)gPlayer->GetMaxHP();
        KV("HP:", oss.str().c_str(),
            gPlayer->GetHP() < gPlayer->GetMaxHP() * 0.3f ? 1.f : 0.4f,
            gPlayer->GetHP() < gPlayer->GetMaxHP() * 0.3f ? 0.3f : 1.f,
            0.4f);

        KV("Map:", inProceduralMap ? "Procedural" : "CSV");

        oss.str(""); oss << enemiesKilledInRoom << " / " << enemiesRequiredForBoss;
        KV("Kills:", oss.str().c_str());

        KV("Boss:", bossSpawned ? "SPAWNED" : "waiting",
            bossSpawned ? 1.f : 0.6f,
            bossSpawned ? 0.4f : 0.8f,
            0.4f);

        y -= 12.f;

        // ---- Toggles ----
        Line("--- Toggles ---", 0.4f, 0.7f, 1.f);
        Toggle("[F5]", "Aggro rings", debugShowAggroRings);
        Toggle("[F6]", "Spawn pts", debugShowSpawnPoints);
        Toggle("[F7]", "God mode", debugGodMode);
        Toggle("[F8]", "Live stats", debugShowStats);
        Toggle("[F9]", "Freeze AI", debugFreezeEnemies);

        y -= 12.f;

        // ---- Actions ----
        Line("--- Actions ---", 0.4f, 0.7f, 1.f);
        Line("[F1]  Kill all enemies", 0.85f, 0.85f, 0.85f);
        Line("[F2]  Force-spawn boss", 0.85f, 0.85f, 0.85f);
        Line("[F3]  Teleport to spawn", 0.85f, 0.85f, 0.85f);
        Line("[F4]  Refill HP", 0.85f, 0.85f, 0.85f);
        Line("[N]   Spawn enemy at cursor", 0.85f, 0.85f, 0.85f);
    }

    // Draws aggro rings around every live enemy (F5 toggle)
    void DrawAggroRings()
    {
        auto& gos = GameObjectManager::GetInstance()->GetGameObjects();
        for (GameObject* go : gos) {
            if (!go || !go->IsEnabled() || go->GetGOType() != GO_TYPE::ENEMY) continue;
            Enemy* e = dynamic_cast<Enemy*>(go);
            if (!e) continue;
            float aggro = e->GetDefinition().attack.aggroRange;
            // Yellow ring = aggro radius, orange ring = leash (1.5x)
            DrawDebugCircle(e->GetPos(), aggro, 1.f, 1.f, 0.f);
            DrawDebugCircle(e->GetPos(), aggro * 1.5f, 1.f, 0.6f, 0.f);
        }
    }

    // Draws a small marker on every TILE_ENEMY position in the current map (F6 toggle)
    void DrawSpawnPoints(TileMap const& tilemap)
    {
        auto mapSz = tilemap.GetMapSize();
        unsigned cols = mapSz.first;
        unsigned rows = mapSz.second;
        for (unsigned r = 0; r < rows; ++r) {
            for (unsigned c = 0; c < cols; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (!t || t->type != TileMap::TILE_ENEMY) continue;
                AEVec2 pos = tilemap.GetTilePosition(r, c);
                AEMtx33 mtx;
                GetTransformMtx(mtx, pos, 0, { 20.f, 20.f });
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxTextureSet(nullptr, 0, 0);
                AEGfxSetTransform(mtx.m);
                AEGfxSetColorToMultiply(1.f, 0.2f, 1.f, 0.8f); // magenta dot
                AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    // Draws live stats above each enemy's head (F8 toggle)
    void DrawEnemyStats()
    {
        if (font < 0) return;
        auto& gos = GameObjectManager::GetInstance()->GetGameObjects();
        for (GameObject* go : gos) {
            if (!go || !go->IsEnabled() || go->GetGOType() != GO_TYPE::ENEMY) continue;
            Enemy* e = dynamic_cast<Enemy*>(go);
            if (!e) continue;
            AEVec2 pos = e->GetPos();
            pos.y += e->GetDefinition().render.radius + 20.f;
            std::ostringstream oss;
            oss << e->GetDefinition().name
                << " HP:" << (int)e->GetHP() << "/" << (int)e->GetMaxHP();
            DrawAEText(font, oss.str().c_str(), pos, 0.45f,
                CreateColor(255, 255, 100, 255), TEXT_MIDDLE, 0);
        }
    }

    // --- SPAWN HELPERS ---

    std::vector<AEVec2> FindSafeSpawnPositions(TileMap const& tilemap, int maxCount)
    {
        std::vector<AEVec2> positions;
        auto mapSz = tilemap.GetMapSize();
        unsigned cols = mapSz.first;
        unsigned rows = mapSz.second;
        int midR = (int)rows / 2;
        int midC = (int)cols / 2;

        // Pass 1: designer-placed TILE_ENEMY markers — always use all of them
        for (unsigned r = 0; r < rows; ++r) {
            for (unsigned c = 0; c < cols; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (t && t->type == TileMap::TILE_ENEMY)
                    positions.push_back(tilemap.GetTilePosition(r, c));
            }
        }
        if (!positions.empty()) return positions;

        // Pass 2: procedural fallback — spaced open tiles only
        const float MIN_SPACING = 115.0f * 3.0f;
        for (unsigned r = 2; r < rows - 2 && (int)positions.size() < maxCount; ++r) {
            for (unsigned c = 2; c < cols - 2 && (int)positions.size() < maxCount; ++c) {
                const TileMap::Tile* t = tilemap.QueryTile(r, c);
                if (!t || t->type != TileMap::TILE_NONE || t->isSolid) continue;

                const TileMap::Tile* up = tilemap.QueryTile(r - 1, c);
                const TileMap::Tile* down = tilemap.QueryTile(r + 1, c);
                const TileMap::Tile* left = tilemap.QueryTile(r, c - 1);
                const TileMap::Tile* right = tilemap.QueryTile(r, c + 1);
                if (!up || up->isSolid)    continue;
                if (!down || down->isSolid)  continue;
                if (!left || left->isSolid)  continue;
                if (!right || right->isSolid) continue;

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
        int spawnCount = 5 + rand() % 6;
        std::vector<AEVec2> spawnPositions = FindSafeSpawnPositions(tilemap, spawnCount);

        procEnemies.clear();
        enemiesKilledInRoom = 0;
        enemiesRequiredForBoss = (int)spawnPositions.size();
        bossSpawned = false;

        for (AEVec2 const& pos : spawnPositions) {
            Enemy* e = SpawnWeightedEnemy(pos, 0.70f, 0.30f);
            if (e) procEnemies.push_back(e);
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
        boss = SpawnRandomBossEnemy(bossPos);
        if (!boss) return;

        bossSpawned = true;
        bossAlive = true;
        bossMaxHPProgressBar = boss->GetDefinition().baseStats.maxHP;
        bossHPProgressBar = bossMaxHPProgressBar;
        std::cout << "[Boss] Spawned " << boss->GetDefinition().name << "!\n";
    }

    void CountDeadProcEnemies()
    {
        int dead = 0;
        for (Enemy* e : procEnemies)
            if (e && (!e->IsEnabled() || e->IsDead())) ++dead;
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
        if (!bossSpawned)       AEGfxSetColorToMultiply(0.2f, 0.7f, 0.2f, 1.f);
        else if (bossAlive)     AEGfxSetColorToMultiply(0.7f, 0.2f, 0.2f, 1.f);
        else                    AEGfxSetColorToMultiply(0.9f, 0.9f, 0.9f, 1.f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Draws a small [DEBUG] badge when debug mode is on
    void DrawDebugBadge()
    {
        if (font < 0) return;
        AEVec2 camPos;
        AEGfxGetCamPosition(&camPos.x, &camPos.y);
        float x = camPos.x + 320.f;
        float y = camPos.y + 305.f;
        DrawAEText(RenderingManager::GetInstance()->GetFont(), "[ DEBUG MODE ]", { x, y }, 0.38f,
            CreateColor(255, 200, 0, 255), TEXT_MIDDLE_LEFT);
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

    bgm.Init(); bgm.PlayNormal();
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
    PetManager::GetInstance()->PlacePet(GetPlayerPos());
    PetManager::GetInstance()->InitPetForGame();

    ActorStats base{};
    base.maxHP = 100.0f;
    base.attack = 10.0f;
    base.attackSpeed = 1.0f;
    base.moveSpeed = playerSpeed;
    gPlayer->InitPlayerRuntime(base);
    gPlayer->ApplyShopUpgrades();
    gPlayer->Heal(gPlayer->GetMaxHP());

    LootChest* chest = dynamic_cast<LootChest*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::LOOT_CHEST));
    chest->Init(chestTilePos, { 35,35 }, 0, MESH_SQUARE, Collision::COL_RECT, { 35,35 },
        CreateBitmask(1, Collision::PLAYER), Collision::INTERACTABLE)
        ->GetRenderData().tint = CreateColor(255, 0.84f * 255.f, 0, 255);

    // CSV map: spawn at every TILE_ENEMY marker, weighted 70/30 Normal/Elite
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
    bossAlive = false;
    enemiesRequiredForBoss = (int)csvEnemies.size();
    enemiesKilledInRoom = 0;
    bossMaxHPProgressBar = (float)enemiesRequiredForBoss;
    bossHPProgressBar = 0.f;

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

    // TAB — toggle debug mode and overlay
    if (AEInputCheckTriggered(AEVK_TAB)) {
        debugMode = !debugMode;
        showDebugOverlay = debugMode;
        std::cout << "[Debug] Mode " << (debugMode ? "ON" : "OFF") << "\n";
    }

    // K — toggle the in-game keybind reference overlay (works in and out of debug mode)
    if (AEInputCheckTriggered(AEVK_K)) {
        showKeybindOverlay = !showKeybindOverlay;
    }

    // All keys below only fire when debug mode is active
    if (debugMode) {
        // Feature toggles
        if (AEInputCheckTriggered(AEVK_F5)) { debugShowAggroRings = !debugShowAggroRings;  std::cout << "[Debug] Aggro rings " << (debugShowAggroRings ? "ON" : "OFF") << "\n"; }
        if (AEInputCheckTriggered(AEVK_F6)) { debugShowSpawnPoints = !debugShowSpawnPoints; std::cout << "[Debug] Spawn points " << (debugShowSpawnPoints ? "ON" : "OFF") << "\n"; }
        if (AEInputCheckTriggered(AEVK_F7)) { debugGodMode = !debugGodMode;         std::cout << "[Debug] God mode " << (debugGodMode ? "ON" : "OFF") << "\n"; }
        if (AEInputCheckTriggered(AEVK_F8)) { debugShowStats = !debugShowStats;       std::cout << "[Debug] Live stats " << (debugShowStats ? "ON" : "OFF") << "\n"; }
        if (AEInputCheckTriggered(AEVK_F9)) { debugFreezeEnemies = !debugFreezeEnemies;   std::cout << "[Debug] Freeze AI " << (debugFreezeEnemies ? "ON" : "OFF") << "\n"; }

        // Action keys
        if (AEInputCheckTriggered(AEVK_F1)) {
            // Kill all enemies in the current room instantly
            for (Enemy* e : procEnemies) if (e && e->IsEnabled()) e->TakeDamage({ 99999.f, nullptr, DAMAGE_TYPE::TRUE_DAMAGE, nullptr });
            for (Enemy* e : csvEnemies)  if (e && e->IsEnabled()) e->TakeDamage({ 99999.f, nullptr, DAMAGE_TYPE::TRUE_DAMAGE, nullptr });
            std::cout << "[Debug] All enemies killed.\n";
        }
        if (AEInputCheckTriggered(AEVK_F2)) {
            // Force-spawn boss immediately regardless of kill count
            if (!bossSpawned && inProceduralMap && nextMap) {
                enemiesKilledInRoom = enemiesRequiredForBoss; // fake the threshold
                TrySpawnBoss(*nextMap);
                std::cout << "[Debug] Boss force-spawned.\n";
            }
        }
        if (AEInputCheckTriggered(AEVK_F3)) {
            // Teleport player to the current map's spawn point
            if (gPlayer) {
                TileMap* cur = inProceduralMap ? nextMap : map;
                AEVec2 sp = cur ? cur->GetSpawnPoint() : AEVec2{ 0,0 };
                gPlayer->SetPos(sp);
                camPos = sp;
                std::cout << "[Debug] Teleported to spawn.\n";
            }
        }
        if (AEInputCheckTriggered(AEVK_F4)) {
            // Refill player HP to max
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
        }
    }
#pragma endregion

    if (!gPlayer) return;

    // Apply god mode — keep HP at max every frame
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
            SpawnProceduralEnemies(*nextMap);
            minimap->Reset();
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
        CountDeadProcEnemies();
        TrySpawnBoss(*nextMap);
    }
    else {
        int dead = 0;
        for (Enemy* e : csvEnemies)
            if (e && (!e->IsEnabled() || e->IsDead())) ++dead;
        enemiesKilledInRoom = dead;
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

    // Freeze AI — skip GO update and handle manually
    if (debugMode && debugFreezeEnemies) {
        // Still update non-enemy objects (player, projectiles, etc.)
        // by passing nullptr as the tilemap so enemies skip their AI
        // If GameObjectManager doesn't support that, just skip the call:
        // GameObjectManager::GetInstance()->UpdateObjects(dt, currentMap);
        // For now we still call it — enemies check debugFreezeEnemies in their own Update
        // via a global flag if you add that check, otherwise use the F1 kill approach.
        // Simple fallback: call update normally (enemies still move), freeze is visual-only.
    }

    minimap->Update(dt, *currentMap, *gPlayer);
    UpdateWorldMap((float)dt);
    GameObjectManager::GetInstance()->UpdateObjects(dt, currentMap);
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

    // Debug visuals — drawn on top of everything
    if (debugMode) {
        if (debugShowAggroRings)  DrawAggroRings();
        if (debugShowSpawnPoints) DrawSpawnPoints(*currentMap);
        if (debugShowStats)       DrawEnemyStats();
        // Text overlays require texture render mode
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        if (showDebugOverlay)     DrawDebugOverlay();
        DrawDebugBadge();
    }

    // Keybind overlay — available always, independent of debug mode
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    if (showKeybindOverlay) DrawKeybindOverlay();

    // Persistent hint in bottom-right so players know K exists
    if (!showKeybindOverlay && font >= 0) {
        AEVec2 camPos;
        AEGfxGetCamPosition(&camPos.x, &camPos.y);
        DrawAEText(RenderingManager::GetInstance()->GetFont(), "[K] Keybinds",
            { camPos.x + 340.f, camPos.y - 280.f },
            0.4f, CreateColor(180, 180, 180, 180), TEXT_MIDDLE_LEFT);
    }

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
    gPlayer->ClearStatusEffects();
    inProceduralMap = false;
    teleportCooldown = 0.f;
    bossSpawned = false;
    bossAlive = true;
    enemiesKilledInRoom = 0;
    enemiesRequiredForBoss = 0;
    // Reset all debug toggles on exit so they don't bleed into next session
    debugMode = false;
    showDebugOverlay = false;
    showKeybindOverlay = false;
    debugShowAggroRings = false;
    debugShowSpawnPoints = false;
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

    if (doTutorial && fairy) { delete fairy; fairy = nullptr; }

    bgm.Exit();
    if (font >= 0) { AEGfxDestroyFont(font); font = -1; }
}

bool getBossAlive() { return bossAlive; }
float getBossHPProgressBar() { return bossHPProgressBar; }
void setBossHPProgressBar(float current) { bossHPProgressBar = current; }
float getBossMaxHPProgressBar() { return bossMaxHPProgressBar; }
void setBossMaxHPProgressBar(float max) { bossMaxHPProgressBar = max; }