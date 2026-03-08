#include "Minimap.h"
#include "../GameObjects/GameObjectManager.h"
#include "../Helpers/RenderUtils.h"
#include "../RenderingManager.h"
#include "../Helpers/MatrixUtils.h"
#include "../Helpers/Vec2Utils.h"

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

// Helper to clamp minimap positions so nothing bleeds outside
inline void ClampToMinimap(float& x, float& y, float mmX, float mmY, AEVec2 const& size) {
    float halfW = size.x * 0.5f;
    float halfH = size.y * 0.5f;

    float minX = mmX - halfW;
    float maxX = mmX + halfW;
    float minY = mmY - halfH;
    float maxY = mmY + halfH;

    if (x < minX) x = minX;
    else if (x > maxX) x = maxX;

    if (y < minY) y = minY;
    else if (y > maxY) y = maxY;
}


void Minimap::Update(double dt, TileMap const& tilemap, Player const& player)
{
    AEVec2 p = player.GetPos();
    AEVec2 const& mapSize{ tilemap.GetFullMapSize() };
    AEVec2 fogTileSize{ (mapSize / FOG_GRID_SIZE) };
    AEVec2 mapOffset = tilemap.GetOffset();

    // Scale reveal radius proportionally to map size
    float scaledRevealRadius = revealRadiusWorld * (mapSize.x / 3000.f);

    for (int x = 0; x < FOG_GRID_SIZE; ++x) {
        for (int y = 0; y < FOG_GRID_SIZE; ++y) {
            if (regenTimerGrid[x][y] > 0.0f) regenTimerGrid[x][y] -= (float)dt;
            else {
                discoveryGrid[x][y] -= FOG_REGEN_RATE * (float)dt;
                if (discoveryGrid[x][y] < 0.0f) discoveryGrid[x][y] = 0.0f;
            }

            float tileWorldX = mapOffset.x + (x * fogTileSize.x) - mapSize.x * 0.5f + (fogTileSize.x * 0.5f);
            float tileWorldY = mapOffset.y + (y * fogTileSize.y) - mapSize.y * 0.5f + (fogTileSize.y * 0.5f);

            float dx = p.x - tileWorldX;
            float dy = p.y - tileWorldY;

            if ((dx * dx + dy * dy) < (scaledRevealRadius * scaledRevealRadius)) {
                discoveryGrid[x][y] = 1.0f;
                regenTimerGrid[x][y] = REGEN_DELAY;
            }
        }
    }
}

void Minimap::Render(TileMap const& tilemap, Player const& player) const
{
    AEVec2 const& mapSize{ tilemap.GetFullMapSize() };

    float scaleX = size.x / mapSize.x, scaleY = size.y / mapSize.y;
    float mmX = (float)AEGfxGetWinMaxX() - size.x * 0.5f - minimapMargin;
    float mmY = (float)AEGfxGetWinMaxY() - size.y * 0.5f - minimapMargin - 10.0f;

    // The map's world offset — needed to correctly place the player and objects
    // on the minimap regardless of whether it's the CSV or procedural map
    AEVec2 mapOffset = tilemap.GetOffset();

    // Draw the tile map (TileMap::Render handles posOffset internally)
    tilemap.Render({ mmX, mmY }, 0, { scaleX, scaleY }, true);

    // Draw game objects — subtract mapOffset so positions are relative to the map origin
    std::vector<GameObject*> const& gos{ GameObjectManager::GetInstance()->GetGameObjects() };
    for (GameObject const* go : gos) {
        if (go->GetGOType() < GO_TYPE_MINIMAP_RENDERABLE || !go->IsEnabled()) continue;

        AEVec2 relativePos = go->GetPos() - mapOffset;
        DrawTintedMesh(
            GetTransformMtx(AEVec2{ mmX, mmY } + relativePos * AEVec2{ scaleX, scaleY }, 0, { gameobjectRadius * 2, gameobjectRadius * 2 }),
            circleMesh, nullptr,
            gameobjectTints.find(go->GetGOType()) != gameobjectTints.end() ? gameobjectTints.at(go->GetGOType()) : Color{ 255,255,255,255 },
            255
        );
    }

    // Draw player arrow — subtract mapOffset so it tracks correctly on both maps
    AEVec2 p = player.GetPos();
    float cx = mmX + (p.x - mapOffset.x) * scaleX;
    float cy = mmY + (p.y - mapOffset.y) * scaleY;
    AEVec2 d{ player.GetMoveDirNorm() };

    if (d.x || d.y) {
        float px = -d.y, py = d.x; // perpendicular vector for arrow width
        float dotSize = gameobjectRadius * scaleX;

        float ax = cx + d.x * (dotSize + 5.0f), ay = cy + d.y * (dotSize + 5.0f);
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

    // Draw fog — also offset by mapOffset so fog aligns with the correct map
    AEVec2 fogTileSize{ (mapSize / FOG_GRID_SIZE) };
    AEMtx33 s{};
    AEMtx33Scale(&s, fogTileSize.x * scaleX, fogTileSize.y * scaleY);
    for (int x = 0; x < FOG_GRID_SIZE; x++) {
        for (int y = 0; y < FOG_GRID_SIZE; y++) {
            if (discoveryGrid[x][y] < 1.0f) {
                AEMtx33 t, f;
                float oX = x * fogTileSize.x + fogTileSize.x * 0.5f;
                float oY = y * fogTileSize.y + fogTileSize.y * 0.5f;

                AEMtx33Trans(&t, mmX + (oX - mapSize.x * 0.5f) * scaleX, mmY + (oY - mapSize.y * 0.5f) * scaleY);
                AEMtx33Concat(&f, &t, &s); AEGfxSetTransform(f.m);
                AEGfxSetColorToMultiply(0.1f, 0.1f, 0.1f, 1.0f - discoveryGrid[x][y]);
                AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
            }
        }
    }

    // Draw minimap border frame
    AEMtx33 bS, bT, bF;
    AEMtx33Scale(&bS, size.x + 10.f, size.y + 10.f);
    AEMtx33Trans(&bT, mmX, mmY);
    AEMtx33Concat(&bF, &bT, &bS); AEGfxSetTransform(bF.m);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);
}