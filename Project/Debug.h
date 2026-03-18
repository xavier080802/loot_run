#pragma once

#include "../Project/Tilemap.h"
#include "../Project/Actor/Player.h"
#include "../Project/Actor/Enemy.h"

struct DebugContext
{
    AEGfxVertexList* squareMesh = nullptr;
    s8      font = -1;
    Player* gPlayer = nullptr;

    bool showDebugOverlay = false;
    bool showKeybindOverlay = false;

    bool debugGodMode = false;
    bool debugShowStats = false;
    bool debugFreezeEnemies = false;

    bool  inProceduralMap = false;
    bool  bossSpawned = false;
    int   enemiesKilledInRoom = 0;
    int   enemiesRequiredForBoss = 0;
};

void DrawPanel(const DebugContext& ctx, float screenX, float screenY, float w, float h);
void DrawKeybindOverlay(const DebugContext& ctx);
void DrawDebugOverlay(const DebugContext& ctx);
void DrawEnemyStats(const DebugContext& ctx);