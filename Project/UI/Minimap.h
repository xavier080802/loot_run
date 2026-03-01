#ifndef _MINIMAP_H_
#define _MINIMAP_H_
#include "AEEngine.h"
#include "../TileMap.h"
#include "../GameObjects/GameObject.h"
#include "../Actor/Player.h"
#include "../Helpers/RenderUtils.h"

class Minimap
{
public:
    Minimap();
    void Reset();
    void Update(double dt, TileMap const& tilemap, Player const& player);
    void Render(TileMap const& tilemap, Player const& player) const;

private:
    //Tints for specific gameobjects
    const std::map<GO_TYPE, Color> gameobjectTints{
        std::make_pair(GO_TYPE::PLAYER, Color{255,255,255,255}),
        std::make_pair(GO_TYPE::ENEMY, Color{255,0,0,255}),
        std::make_pair(GO_TYPE::LOOT_CHEST, Color{255,214,255,255}),
    };

    AEGfxVertexList* squareMesh{}, * circleMesh{}, * borderMesh{};

    // --- FOG OF WAR SETTINGS ---
    static const int FOG_GRID_SIZE{ 40 };
    float discoveryGrid[FOG_GRID_SIZE][FOG_GRID_SIZE]{};      // 1.0 = Visible, 0.0 = Dark
    float regenTimerGrid[FOG_GRID_SIZE][FOG_GRID_SIZE]{};     // Seconds before fog returns
    const float REGEN_DELAY = 5.0f;                         // Stay revealed for 5s
    const float FOG_REGEN_RATE = 0.3f;                      // How fast it fades back to dark
    const float revealRadiusWorld = 180.0f;

    // --- MINIMAP UI SETTINGS ---
    const AEVec2 size{ 200.f, 200.f };
    const float minimapMargin = 20.0f;
    const float gameobjectRadius = 4.375f;

};
#endif // !_MINIMAP_H_
