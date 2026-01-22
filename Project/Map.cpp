#include "Map.h"

void InitTutorial(MapData& map) {
    map.walls.clear();

    const float ROOM_SIZE = 400.0f;
    const float WALL_THICK = 20.0f;
    const float CORRIDOR_WIDTH = 100.0f;

    // --- Rooms (outer walls) ---

    // Room 1 (Start)
    map.walls.push_back({ {-600.0f, 600.0f + ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // top
    map.walls.push_back({ {-600.0f, 600.0f - ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // bottom
    map.walls.push_back({ {-600.0f - ROOM_SIZE / 2, 600.0f}, WALL_THICK, ROOM_SIZE }); // left
    map.walls.push_back({ {-600.0f + ROOM_SIZE / 2, 600.0f}, WALL_THICK, ROOM_SIZE }); // right

    // Room 2
    map.walls.push_back({ {0.0f, 600.0f + ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // top
    map.walls.push_back({ {0.0f, 600.0f - ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // bottom
    map.walls.push_back({ {-ROOM_SIZE / 2, 600.0f}, WALL_THICK, ROOM_SIZE }); // left
    map.walls.push_back({ {ROOM_SIZE / 2, 600.0f}, WALL_THICK, ROOM_SIZE }); // right

    // Room 3 (Enemy)
    map.walls.push_back({ {600.0f, 600.0f + ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // top
    map.walls.push_back({ {600.0f, 600.0f - ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // bottom
    map.walls.push_back({ {600.0f - ROOM_SIZE / 2, 600.0f}, WALL_THICK, ROOM_SIZE }); // left
    map.walls.push_back({ {600.0f + ROOM_SIZE / 2, 600.0f}, WALL_THICK, ROOM_SIZE }); // right

    // Room 4 (Chest)
    map.walls.push_back({ {600.0f, 0.0f + ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // top
    map.walls.push_back({ {600.0f, 0.0f - ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // bottom
    map.walls.push_back({ {600.0f - ROOM_SIZE / 2, 0.0f}, WALL_THICK, ROOM_SIZE }); // left
    map.walls.push_back({ {600.0f + ROOM_SIZE / 2, 0.0f}, WALL_THICK, ROOM_SIZE }); // right

    // Room 5 (Enemy)
    map.walls.push_back({ {0.0f, 0.0f + ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // top
    map.walls.push_back({ {0.0f, 0.0f - ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // bottom
    map.walls.push_back({ {-ROOM_SIZE / 2, 0.0f}, WALL_THICK, ROOM_SIZE }); // left
    map.walls.push_back({ {ROOM_SIZE / 2, 0.0f}, WALL_THICK, ROOM_SIZE }); // right

    // Room 6 (Enemy + Door)
    map.walls.push_back({ {-600.0f, 0.0f + ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // top
    map.walls.push_back({ {-600.0f, 0.0f - ROOM_SIZE / 2}, ROOM_SIZE, WALL_THICK }); // bottom
    map.walls.push_back({ {-600.0f - ROOM_SIZE / 2, 0.0f}, WALL_THICK, ROOM_SIZE }); // left
    map.walls.push_back({ {-600.0f + ROOM_SIZE / 2, 0.0f}, WALL_THICK, ROOM_SIZE }); // right

    // --- Corridors ---

    // 3 ? 4 vertical corridor
    map.walls.push_back({ {600.0f + ROOM_SIZE / 2 + CORRIDOR_WIDTH / 2, 300.0f}, CORRIDOR_WIDTH, ROOM_SIZE }); // right wall
    map.walls.push_back({ {600.0f + ROOM_SIZE / 2 - CORRIDOR_WIDTH / 2, 300.0f}, CORRIDOR_WIDTH, ROOM_SIZE }); // left wall

    // 4 ? 5 horizontal corridor
    map.walls.push_back({ {300.0f, -CORRIDOR_WIDTH / 2}, ROOM_SIZE, CORRIDOR_WIDTH }); // top wall
    map.walls.push_back({ {300.0f, CORRIDOR_WIDTH / 2}, ROOM_SIZE, CORRIDOR_WIDTH }); // bottom wall

    // 5 ? 6 horizontal corridor
    map.walls.push_back({ {-300.0f, -CORRIDOR_WIDTH / 2}, ROOM_SIZE, CORRIDOR_WIDTH }); // top wall
    map.walls.push_back({ {-300.0f, CORRIDOR_WIDTH / 2}, ROOM_SIZE, CORRIDOR_WIDTH }); // bottom wall

    // --- Objects ---
    map.startPos = { -600.0f, 600.0f };       // Player starts in Room 1
    map.chestPos = { 600.0f, 0.0f };          // Chest in Room 4
    map.enemy1Pos = { 600.0f, 600.0f };       // Enemy in Room 3
    map.enemy2Pos = { 0.0f, 0.0f };           // Enemy in Room 5
    map.doorPos = { -600.0f, 0.0f };          // Door in Room 6
}
