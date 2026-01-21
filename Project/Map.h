#ifndef MAP_H
#define MAP_H

#include "AEEngine.h"
#include <vector>

struct Wall {
    AEVec2 position;
    float width;
    float height;
};

struct MapData {
    std::vector<Wall> walls;
    AEVec2 chestPos;
    AEVec2 enemy1Pos;
    AEVec2 enemy2Pos;
    AEVec2 startPos;
};

// Function to initialize the specific layout you provided
void InitTutorial(MapData& map);

#endif
