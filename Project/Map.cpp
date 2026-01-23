#include "Map.h"

void InitTutorial(MapData& map)
{
    map.walls.clear();

    // --- The "Suited" Size for 2000x2000 ---
    const float ROOM_SIZE = 700.0f;
    const float WALL_THICK = 25.0f;
    const float CORRIDOR_GAP = 160.0f;
    const float HALF = ROOM_SIZE * 0.5f;

    // Centers are placed to allow for the 700 size + tunnels
    // X: -800, 0, 800 | Y: 600, -600
    AEVec2 r1 = { -800,  600 }; // Start
    AEVec2 r2 = { 0,  600 };
    AEVec2 r3 = { 800,  600 }; // Enemy 1
    AEVec2 r4 = { 800, -600 }; // Chest
    AEVec2 r5 = { 0, -600 }; // Enemy 2
    AEVec2 r6 = { -800, -600 }; // Boss Room / Exit

    auto AddRoom = [&](AEVec2 c, bool openTop, bool openBottom, bool openLeft, bool openRight)
        {
            float seg = (ROOM_SIZE - CORRIDOR_GAP) * 0.5f;
            float offset = (ROOM_SIZE - seg) * 0.5f;

            // Top Wall
            if (!openTop) map.walls.push_back({ { c.x, c.y + HALF }, ROOM_SIZE, WALL_THICK });
            else {
                map.walls.push_back({ { c.x - offset, c.y + HALF }, seg, WALL_THICK });
                map.walls.push_back({ { c.x + offset, c.y + HALF }, seg, WALL_THICK });
            }
            // Bottom Wall
            if (!openBottom) map.walls.push_back({ { c.x, c.y - HALF }, ROOM_SIZE, WALL_THICK });
            else {
                map.walls.push_back({ { c.x - offset, c.y - HALF }, seg, WALL_THICK });
                map.walls.push_back({ { c.x + offset, c.y - HALF }, seg, WALL_THICK });
            }
            // Left Wall
            if (!openLeft) map.walls.push_back({ { c.x - HALF, c.y }, WALL_THICK, ROOM_SIZE });
            else {
                map.walls.push_back({ { c.x - HALF, c.y - offset }, WALL_THICK, seg });
                map.walls.push_back({ { c.x - HALF, c.y + offset }, WALL_THICK, seg });
            }
            // Right Wall
            if (!openRight) map.walls.push_back({ { c.x + HALF, c.y }, WALL_THICK, ROOM_SIZE });
            else {
                map.walls.push_back({ { c.x + HALF, c.y - offset }, WALL_THICK, seg });
                map.walls.push_back({ { c.x + HALF, c.y + offset }, WALL_THICK, seg });
            }
        };

    // --- Build Rooms (Snake Path 1->2->3->4->5->6) ---
    AddRoom(r1, false, false, false, true);
    AddRoom(r2, false, false, true, true);
    AddRoom(r3, false, true, true, false);
    AddRoom(r4, true, false, true, false); // Chest Room
    AddRoom(r5, false, false, true, true);
    AddRoom(r6, false, false, false, true);  // Boss Room Access from Room 5

    // --- Corridor Tunnels (Closing the Gaps) ---
    float tHalf = CORRIDOR_GAP * 0.5f;
    float hDist = 800.0f - ROOM_SIZE; // Distance between horizontal centers (800-700 = 100)
    float vDist = 1200.0f - ROOM_SIZE; // Distance between vertical centers (600 - -600 = 1200)

    // Tunnels Row 1 (1->2, 2->3)
    map.walls.push_back({ { -400, r1.y + tHalf }, hDist, WALL_THICK });
    map.walls.push_back({ { -400, r1.y - tHalf }, hDist, WALL_THICK });
    map.walls.push_back({ {  400, r2.y + tHalf }, hDist, WALL_THICK });
    map.walls.push_back({ {  400, r2.y - tHalf }, hDist, WALL_THICK });

    // Vertical Tunnel (3->4)
    map.walls.push_back({ { r3.x + tHalf, 0 }, WALL_THICK, vDist });
    map.walls.push_back({ { r3.x - tHalf, 0 }, WALL_THICK, vDist });

    // Tunnels Row 2 (4->5, 5->6)
    map.walls.push_back({ {  400, r4.y + tHalf }, hDist, WALL_THICK });
    map.walls.push_back({ {  400, r4.y - tHalf }, hDist, WALL_THICK });
    map.walls.push_back({ { -400, r5.y + tHalf }, hDist, WALL_THICK });
    map.walls.push_back({ { -400, r5.y - tHalf }, hDist, WALL_THICK });

    // --- Entity Placement ---
    map.startPos = r1;
    map.enemy1Pos = r3;
    map.chestPos = r4;
    map.enemy2Pos = r5;
    map.doorPos = r6;
}