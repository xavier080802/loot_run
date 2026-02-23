#include "Minimap.h"
#include "GameObjects/GameObjectManager.h"
#include "Helpers/RenderUtils.h"
#include "RenderingManager.h"
#include "Helpers/MatrixUtils.h"
#include "Helpers/Vec2Utils.h"

Minimap::Minimap()
{
    circleMesh = RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE);
    squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
    borderMesh = RenderingManager::GetInstance()->GetMesh(MESH_BORDER);
}

void Minimap::Reset() {
    // Start with a fully hidden map
    for (int i = 0; i < FOG_GRID_SIZE; i++) {
        for (int j = 0; j < FOG_GRID_SIZE; j++) {
            discoveryGrid[i][j] = 0;
            regenTimerGrid[i][j] = 0;
        }
    }
}

void Minimap::Update(double dt, TileMap const& tilemap, Player const& player)
{
    // Logic to calculate which fog tiles are near the player and reveal them
    AEVec2 p = player.GetPos();
    AEVec2 const& mapSize{ tilemap.GetFullMapSize() };
    AEVec2 fogTileSize{ (mapSize / FOG_GRID_SIZE) }; //world size
    
    for (int x = 0; x < FOG_GRID_SIZE; ++x) {
        for (int y = 0; y < FOG_GRID_SIZE; ++y) {
            if (regenTimerGrid[x][y] > 0.0f) regenTimerGrid[x][y] -= dt;
            else {
                discoveryGrid[x][y] -= FOG_REGEN_RATE * dt;
                if (discoveryGrid[x][y] < 0.0f) discoveryGrid[x][y] = 0.0f;
            }
            float tileWorldX = (x * fogTileSize.x) - mapSize.x/2.f + (fogTileSize.x * 0.5f);
            float tileWorldY = (y * fogTileSize.y) - mapSize.y/2.f + (fogTileSize.y * 0.5f);

            float dx = p.x - tileWorldX;
            float dy = p.y - tileWorldY;

            if ((dx * dx + dy * dy) < (revealRadiusWorld * revealRadiusWorld)) {
                discoveryGrid[x][y] = 1.0f;
                regenTimerGrid[x][y] = REGEN_DELAY;
            }
        }
    }
}

void Minimap::Render(TileMap const& tilemap, Player const& player) const
{
    AEVec2 const& mapSize{ tilemap.GetFullMapSize() };

    // --- 1. CALCULATE UI COORDINATES ---
    // scaleX/Y converts 1 unit of world distance to pixels on the minimap
    float scaleX = size.x / mapSize.x;
    float scaleY = size.y / mapSize.y;

    // mmX/mmY is the CENTER of the minimap on your screen
    float mmX = (float)AEGfxGetWinMaxX() - size.x * 0.5f - minimapMargin;
    float mmY = (float)AEGfxGetWinMaxY() - size.y * 0.5f - minimapMargin - 50.f;

    // Render the actual tile layout centered at mmX, mmY
    tilemap.Render({ mmX, mmY }, 0, { scaleX, scaleY }, true);

    // --- 2. RENDER GAME OBJECTS ---
    std::vector<GameObject*> const& gos{ GameObjectManager::GetInstance()->GetGameObjects() };

    // Scale the dots so they stay small even if the map gets huge
    float dotSizeUI = gameobjectRadius * 2.0f;

    for (GameObject const* go : gos) {
        if (go->GetGOType() < GO_TYPE_MINIMAP_RENDERABLE || !go->IsEnabled()) continue;

        // Convert world position to minimap offset
        AEVec2 relativePos = go->GetPos() * AEVec2 { scaleX, scaleY };

        DrawTintedMesh(
            GetTransformMtx(AEVec2{ mmX, mmY } + relativePos, 0, { dotSizeUI, dotSizeUI }),
            circleMesh,
            nullptr,
            gameobjectTints.count(go->GetGOType()) ? gameobjectTints.at(go->GetGOType()) : Color{ 255,255,255,255 },
            255
        );
    }

    // --- 3. RENDER PLAYER & ORIENTATION ---
    AEVec2 p = player.GetPos();
    float cx = mmX + p.x * scaleX;
    float cy = mmY + p.y * scaleY;
    AEVec2 d{ player.GetMoveDirNorm() };

    if (d.x || d.y) {
        float px = -d.y, py = d.x;
        float scaledArrowOffset = (dotSizeUI * 0.5f) + 2.0f;

        float ax = cx + d.x * scaledArrowOffset, ay = cy + d.y * scaledArrowOffset;
        float tipX = ax + d.x * 8.0f, tipY = ay + d.y * 8.0f;
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

    // --- 4. RENDER FOG OF WAR (FIXED SCALE) ---
    // The fog tile size must be scaled down to minimap pixels, not world pixels!
    AEVec2 fogTileSizeUI = (mapSize / (float)FOG_GRID_SIZE) * AEVec2{ scaleX, scaleY };

    AEMtx33 s;
    AEMtx33Scale(&s, fogTileSizeUI.x, fogTileSizeUI.y);

    for (int x = 0; x < FOG_GRID_SIZE; x++) {
        for (int y = 0; y < FOG_GRID_SIZE; y++) {
            if (discoveryGrid[x][y] < 1.0f) {
                AEMtx33 t, f;

                // Calculate corner of minimap and offset by grid index
                float startX = mmX - (size.x * 0.5f);
                float startY = mmY - (size.y * 0.5f);

                float oX = x * fogTileSizeUI.x + (fogTileSizeUI.x * 0.5f);
                float oY = y * fogTileSizeUI.y + (fogTileSizeUI.y * 0.5f);

                AEMtx33Trans(&t, startX + oX, startY + oY);
                AEMtx33Concat(&f, &t, &s);

                AEGfxSetTransform(f.m);
                AEGfxSetColorToMultiply(0.05f, 0.05f, 0.05f, 1.0f - discoveryGrid[x][y]);
                AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    // --- 5. RENDER BORDER ---
    AEMtx33 bS, bT, bF;
    AEMtx33Scale(&bS, size.x + 4.f, size.y + 4.f);
    AEMtx33Trans(&bT, mmX, mmY);
    AEMtx33Concat(&bF, &bT, &bS);
    AEGfxSetTransform(bF.m);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);
}