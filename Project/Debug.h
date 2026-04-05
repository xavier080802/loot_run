#ifndef DEBUG_H_
#define DEBUG_H_

#include "../Project/GameStates/Related/Tilemap.h"
#include "../Project/Actor/Player.h"
#include "../Project/Actor/Enemy.h"

struct DebugContext
{
    AEGfxVertexList* squareMesh = nullptr;
    s8      font = -1;
    Player* gPlayer = nullptr;
    bool    showDebugOverlay = false;
    bool    showKeybindOverlay = false;
    bool    debugGodMode = false;
    bool    debugFreezeEnemies = false;
    bool    debugShowChests = false;
    bool    inProceduralMap = false;
    bool    bossSpawned = false;
    int     enemiesKilledInRoom = 0;
    int     enemiesRequiredForBoss = 0;
    int     totalEnemiesKilled = 0;
    int     totalKillTarget = 0;
    int     csvEnemyCount = 0;
    int     procEnemyCount = 0;
};

void DrawPanel(const DebugContext& ctx, float screenX, float screenY, float w, float h);
void DrawKeybindOverlay(const DebugContext& ctx);
void DrawDebugOverlay(const DebugContext& ctx);
void DrawEnemyStats(const DebugContext& ctx);
void DrawChestHighlights(const DebugContext& ctx);

#endif // DEBUG_H_

