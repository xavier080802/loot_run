#include "Map.h"

void InitTutorial(MapData& map) {
    map.walls.clear();

    Wall topWall;
    topWall.position = { -200.0f, 400.0f }; 
    topWall.width = 1600.0f;
    topWall.height = 20.0f;
    map.walls.push_back(topWall);

    Wall bottomWall;
    bottomWall.position = { 200.0f, -400.0f };
    bottomWall.width = 1600.0f;
    bottomWall.height = 20.0f;
    map.walls.push_back(bottomWall);

    map.chestPos = { 0.0f, 0.0f };
    map.enemy1Pos = { 0.0f, -700.0f };
    map.enemy2Pos = { -800.0f, 800.0f };
    map.startPos = { 800.0f, -800.0f };
}