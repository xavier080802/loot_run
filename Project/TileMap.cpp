#include "TileMap.h"
#include "File/CSV.h"
#include "RenderingManager.h"
#include "Helpers/MatrixUtils.h"
#include "Helpers/RenderUtils.h"
#include "Helpers/CoordUtils.h"
#include "Helpers/Vec2Utils.h"
#include "camera.h"
#include <cstdlib> 

std::map<TileMap::TILE_TYPE, AEGfxTexture*> TileMap::textureMap{};
std::map<TileMap::TILE_TYPE, TileMap::Tile> TileMap::tileMap{};
AEGfxVertexList* TileMap::pMesh{};

// Shortcut aliases to keep the insert calls below readable
using TileTex = std::pair<TileMap::TILE_TYPE, AEGfxTexture*>;
using TilePair = std::pair<TileMap::TILE_TYPE, TileMap::Tile>;


// ============================================================
//  Constructor - CSV-based map
//  Reads a CSV file where each cell is an integer tile type.
//  offset shifts the whole map in world space.
//  tileX / tileY are the pixel dimensions of a single tile.
// ============================================================
TileMap::TileMap(std::string filename, AEVec2 offset, float tileX, float tileY)
    : tileSize{ AEVec2{tileX, tileY} }, posOffset(offset)
{
    // only runs once no matter how many maps create
    if (!textureMap.size()) {
        LoadStatics();
    }

    CSV file{ filename };
    rows = file.GetRows();
    cols = file.GetCols();
    mapSize = { cols * tileX, rows * tileY };

    tiles.reserve(rows);

    // Walk every cell in the CSV and build the 2D tile grid
    for (unsigned r{}; r < rows; ++r) {
        tiles.push_back({});
        tiles[r].reserve(cols);

        for (unsigned c{}; c < cols; ++c) {
            std::string data{ file.GetData(r, c) };

            // Empty cells default to TILE_NONE (passable, no texture)
            if (data == "") {
                tiles[r].push_back(tileMap[TILE_NONE]);
                continue;
            }

            int v = std::stoi(data);
            // Out-of-range values are clamped to TILE_NONE so bad CSV data doesn't crash
            tiles[r].push_back(tileMap[(v >= TILE_NUM ? TILE_NONE : static_cast<TILE_TYPE>(v))]);
        }
    }
}

TileMap::TileMap(AEVec2 offset, float tileX, float tileY)
    : tileSize{ AEVec2{tileX, tileY} }, posOffset(offset), rows{ 0 }, cols{ 0 }
{
    if (!textureMap.size()) {
        LoadStatics();
    }
}


// ============================================================
//  GenerateProcedural
//  Builds a randomised dungeon room using cellular automata.
//  The same seed always produces the same map, which allow
//  recreation of a room without storing the full tile data.
// ============================================================
void TileMap::GenerateProcedural(unsigned int r, unsigned int c, int seed)
{
    rows = r;
    cols = c;
    mapSize = { cols * tileSize.x, rows * tileSize.y };

    srand(seed);

    // Start with all walls, then carve out open space
    tiles.clear();
    tiles.resize(rows, std::vector<Tile>(cols, tileMap[TILE_WALL]));

    // --- Step 1: Random fill ---
    // Border tiles stay as walls permanently; only inner tiles are randomised
    for (unsigned int i = 1; i < rows - 1; ++i) {
        for (unsigned int j = 1; j < cols - 1; ++j) {
            tiles[i][j] = (rand() % 100 < 55) ? tileMap[TILE_NONE] : tileMap[TILE_WALL];
        }
    }

    // --- Step 2: Smooth the map ---
// Each tile checks its 8 neighbours if 5+ are walls, it becomes a wall.
// Running this 3 times turns random noise into proper cave rooms.
    for (int pass = 0; pass < 3; ++pass) {
        std::vector<std::vector<Tile>> next = tiles; // copy so reads don't see this pass's writes
        for (unsigned int i = 1; i < rows - 1; ++i) {
            for (unsigned int j = 1; j < cols - 1; ++j) {
                int wallCount = 0;
                for (int di = -1; di <= 1; ++di)
                    for (int dj = -1; dj <= 1; ++dj)
                        if (tiles[i + di][j + dj].type == TILE_WALL) ++wallCount;

                // majority walls = become a wall, otherwise stay open
                next[i][j] = (wallCount >= 5) ? tileMap[TILE_WALL] : tileMap[TILE_NONE];
            }
        }
        tiles = next;
    }

    // map centre - used as anchor for everything below
    int midR = rows / 2;
    int midC = cols / 2;

    // --- Step 3: Guaranteed 5x5 open center ---
    // Ensure the player always has somewhere safe to spawn regardless of the random seed
    for (int dr = -2; dr <= 2; ++dr)
        for (int dc = -2; dc <= 2; ++dc) {
            int rr = midR + dr, cc = midC + dc;
            // Clamp to inner tiles only so we never overwrite the border wall
            if (rr > 0 && rr < (int)rows - 1 && cc > 0 && cc < (int)cols - 1)
                tiles[rr][cc] = tileMap[TILE_NONE];
        }

    // --- Step 4: Cross corridors ---
    // A horizontal + vertical corridor through the map centre guarantees that
    // the connectors on each edge are always reachable from the spawn point.
    // Only carve walls - already-open tiles are left alone.
    for (unsigned int j = 1; j < cols - 1; ++j)
        if (tiles[midR][j].type == TILE_WALL)
            tiles[midR][j] = tileMap[TILE_NONE];

    for (unsigned int i = 1; i < rows - 1; ++i)
        if (tiles[i][midC].type == TILE_WALL)
            tiles[i][midC] = tileMap[TILE_NONE];

    // --- Step 5: Edge connectors ---
    // Two connector tiles + one open buffer tile on each of the four edges.
    // The double connector acts as a thick doorway so it's hard to miss.
    tiles[midR][0] = tileMap[TILE_CONNECTOR]; // left edge
    tiles[midR][1] = tileMap[TILE_CONNECTOR];
    tiles[midR][2] = tileMap[TILE_NONE];       // open buffer
    tiles[midR][cols - 1] = tileMap[TILE_CONNECTOR]; // right edge
    tiles[midR][cols - 2] = tileMap[TILE_CONNECTOR];
    tiles[midR][cols - 3] = tileMap[TILE_NONE];
    tiles[0][midC] = tileMap[TILE_CONNECTOR]; // top edge
    tiles[1][midC] = tileMap[TILE_CONNECTOR];
    tiles[2][midC] = tileMap[TILE_NONE];
    tiles[rows - 1][midC] = tileMap[TILE_CONNECTOR]; // bottom edge
    tiles[rows - 2][midC] = tileMap[TILE_CONNECTOR];
    tiles[rows - 3][midC] = tileMap[TILE_NONE];

    // --- Step 6: Enemy and chest placement ---
    // Only scatter into open tiles that aren't on the corridors or the safe center.
    // ~4% chance of enemy, ~3% chance of chest per qualifying tile.
    // Start at i=2/j=2 so neighbour reads never go out of bounds.
    // All 4 orthogonal neighbours must be non-solid - stops enemies spawning
    // directly against a wall.
    for (unsigned int i = 2; i < rows - 2; ++i) {
        for (unsigned int j = 2; j < cols - 2; ++j) {
            if (tiles[i][j].type != TILE_NONE) continue;

            bool onCorridor = ((int)i == midR || (int)j == midC);
            int  dr = (int)i - midR, dc = (int)j - midC;
            bool inCenter = (dr >= -2 && dr <= 2 && dc >= -2 && dc <= 2);
            if (onCorridor || inCenter) continue;

            // skip tiles adjacent to any wall
            if (tiles[i - 1][j].isSolid) continue;
            if (tiles[i + 1][j].isSolid) continue;
            if (tiles[i][j - 1].isSolid) continue;
            if (tiles[i][j + 1].isSolid) continue;

            int chance = rand() % 100;
            if (chance < 4) tiles[i][j] = tileMap[TILE_ENEMY];
            else if (chance < 7) tiles[i][j] = tileMap[TILE_CHEST];
        }
    }
}


// ============================================================
//  Render (world-space version)
//  Draws every non-empty tile transformed through the camera.
//  TILE_NONE and tiles with no texture are skipped silently.
// ============================================================
void TileMap::Render() const
{
    for (unsigned int r = 0; r < rows; r++) {
        for (unsigned int c = 0; c < cols; c++) {
            if (tiles[r][c].type == TILE_NONE) continue;

            auto it = textureMap.find(tiles[r][c].type);
            if (it == textureMap.end() || it->second == nullptr) continue;

            f32    rot = 0;
            AEVec2 pos{ GetTilePosition(r, c) };
            AEVec2 scale = tileSize;

            // Apply camera transform so the tile moves with the world view
            GetObjViewFromCamera(&pos, &rot, &scale);
            DrawTintedMesh(GetTransformMtx(pos, rot, scale), pMesh, it->second, { 255,255,255,255 }, 255);
        }
    }
}
// Minimap render - draws the tilemap scaled down into the minimap box.
// offsetPos = minimap centre on screen, scale = shrink factor to fit the box.
// isHud = true skips camera correction since the minimap is fixed to the screen.
void TileMap::Render(AEVec2 offsetPos, float rotOffset, AEVec2 scale, bool isHud) const
{
    for (unsigned int r = 0; r < rows; r++) {
        for (unsigned int c = 0; c < cols; c++) {
            if (tiles[r][c].type == TILE_NONE) continue;
            auto it = textureMap.find(tiles[r][c].type);
            if (it == textureMap.end() || it->second == nullptr) continue;

            f32    rot = rotOffset;
            AEVec2 pos{ GetTilePosition(r, c) * scale };
            AEVec2 tileScale = tileSize;

            if (!isHud) GetObjViewFromCamera(&pos, &rot, &tileScale); // world render only
            DrawTintedMesh(GetTransformMtx(pos + offsetPos, rot, tileScale * scale),
                pMesh, it->second, { 255,255,255,255 }, 255);
        }
    }
}


// Converts a (row, col) index to a world-space position.
// Tiles are anchored at their centre, and row 0 is at the top
// of the map (y increases downward in tile space but the world
// y-axis points up, so we flip with (rows - 1 - rowInd)).
AEVec2 TileMap::GetTilePosition(unsigned rowInd, unsigned colInd) const
{
    AEVec2 out{
        colInd * tileSize.x + tileSize.x * 0.5f,          // centre of column
        (rows - 1 - rowInd) * tileSize.y                  // flip row so row 0 = top
    };
    return out - mapSize * 0.5f + posOffset; // shift so the map is centred at posOffset
}

// Converts a world-space position back to a (col, row) float index.
// The returned x = column index, y = row index.
// Fractional values indicate a position inside a tile - callers that
// want the actual tile should cast to int.
AEVec2 TileMap::GetTileIndFromPos(AEVec2 pos) const
{
    AEVec2 p{ pos + mapSize * 0.5f - posOffset }; // shift into local tile space
    AEVec2 out{
        (f32)((int)(p.x / tileSize.x)),
        (f32)((int)(-(p.y - tileSize.y * 0.5f) / tileSize.y + (rows - 1)))
    };
    return out;
}



// Given a world position, returns the tile sitting there.
// Returns nullptr if the position is outside the map.
TileMap::Tile const* TileMap::QueryTile(AEVec2 pos) const
{
    AEVec2 inds{ GetTileIndFromPos(pos) };
    if (inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) return nullptr;
    return &tiles[(unsigned)inds.y][(unsigned)inds.x];
}

// Same but takes a row/col index directly instead of a world position.
TileMap::Tile const* TileMap::QueryTile(unsigned rowInd, unsigned colInd) const
{
    if (colInd < 0 || colInd >= cols || rowInd < 0 || rowInd >= rows) return nullptr;
    return &tiles[rowInd][colInd];
}

// Same as QueryTile(pos) but also hands back the row/col index,
// so the caller doesn't need to calculate it separately.
std::pair<TileMap::Tile const*, AEVec2> TileMap::QueryTileAndInd(AEVec2 pos) const
{
    AEVec2 inds{ GetTileIndFromPos(pos) };
    //If Index out of bounds, tile should be nullptr
    return std::make_pair((inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) ? nullptr : &tiles[(unsigned)inds.y][(unsigned)inds.x], inds);
}

// Swaps the tile at (row, col) to a new type.
// Returns nullptr if the indices are out of bounds.
TileMap::Tile const* TileMap::ChangeTile(unsigned row, unsigned col, TILE_TYPE newType)
{
    if (col < 0 || col >= cols || row < 0 || row >= rows) return nullptr;
    tiles[row][col] = tileMap[newType];
    return &tiles[row][col];
}



// Returns true if the world position falls on a wall tile or outside the map.
// Treating out-of-bounds as solid stops entities from walking off the edge.
bool TileMap::IsWall(AEVec2 worldPos) const
{
    AEVec2 inds = GetTileIndFromPos(worldPos);
    if (inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) return true;
    return tiles[(unsigned)inds.y][(unsigned)inds.x].type == TILE_WALL;
}

// Returns true if the world position is a connector tile (a room exit/entry portal).
bool TileMap::IsConnector(AEVec2 worldPos) const
{
    AEVec2 inds = GetTileIndFromPos(worldPos);
    if (inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) return false;
    return tiles[(unsigned)inds.y][(unsigned)inds.x].type == TILE_CONNECTOR;
}


// Returns the world position of the first open tile in row 1 (second row from top).
// Used for rooms where the player enters from the top connector.
// Falls back to (row 1, col 0) if no open tile exists in that row.
AEVec2 TileMap::GetSecondRowSpawn() const
{
    if (rows < 2) return { 0, 0 };

    for (unsigned col = 0; col < cols; ++col) {
        if (tiles[1][col].type == TILE_NONE)
            return GetTilePosition(1, col);
    }

    return GetTilePosition(1, 0); // fallback
}

// Finds the best open spawn point near the map centre by spiralling outward.
// Prefers tiles with at least 2 open orthogonal neighbours so the player
// isn't spawned in a dead-end pocket.
AEVec2 TileMap::GetSpawnPoint() const
{
    int midR = (int)rows / 2;
    int midC = (int)cols / 2;

    for (int radius = 0; radius < (int)rows; ++radius) {
        for (int dr = -radius; dr <= radius; ++dr) {
            for (int dc = -radius; dc <= radius; ++dc) {
                // Only check the shell of the current radius, not the interior
                if (abs(dr) != radius && abs(dc) != radius) continue;

                int r = midR + dr;
                int c = midC + dc;

                // Stay away from the border walls
                if (r < 1 || r >= (int)rows - 1 || c < 1 || c >= (int)cols - 1) continue;
                if (tiles[r][c].type != TILE_NONE) continue;

                // Count how many orthogonal neighbours are open
                int openNeighbors = 0;
                if (tiles[r - 1][c].type == TILE_NONE) ++openNeighbors;
                if (tiles[r + 1][c].type == TILE_NONE) ++openNeighbors;
                if (tiles[r][c - 1].type == TILE_NONE) ++openNeighbors;
                if (tiles[r][c + 1].type == TILE_NONE) ++openNeighbors;

                if (openNeighbors >= 2)
                    return GetTilePosition((unsigned)r, (unsigned)c);
            }
        }
    }

    return posOffset; // fallback
}


// ============================================================
//  LoadStatics
//  Called once the first time any TileMap is constructed.
//  Sets up the shared mesh, default collision properties for
//  every tile type, and loads all tile textures.
//
//  Collision defaults: every tile type starts as an OBSTACLE.
//  then explicitly mark the non-solid types (NONE, ENEMY,
//  CHEST, CONNECTOR) as passable.
// ============================================================
void TileMap::LoadStatics()
{
    RenderingManager* rm = RenderingManager::GetInstance();
    pMesh = rm->GetMesh(MESH_SQUARE);

    // Default all types to solid obstacles first
    for (int i{}; i < TILE_NUM; ++i) {
        tileMap.insert(TilePair(
            static_cast<TILE_TYPE>(i),
            { static_cast<TILE_TYPE>(i), Collision::OBSTACLE }
        ));
    }

    // Mark non-solid tile types as passable
    tileMap[TILE_NONE].layer = Collision::NONE;
    tileMap[TILE_NONE].isSolid = false;

    tileMap[TILE_ENEMY].layer = Collision::NONE;  // spawn marker, not a physical wall
    tileMap[TILE_ENEMY].isSolid = false;

    tileMap[TILE_CHEST].layer = Collision::NONE;  // chest is handled by game logic, not tile collision
    tileMap[TILE_CHEST].isSolid = false;

    tileMap[TILE_CONNECTOR].layer = Collision::NONE; // player walks through connectors to change rooms
    tileMap[TILE_CONNECTOR].isSolid = false;

    textureMap.insert(TileTex(TILE_NONE, nullptr));
    textureMap.insert(TileTex(TILE_WALL, rm->LoadTexture("Assets/finn.png")));
    textureMap.insert(TileTex(TILE_DOOR, rm->LoadTexture("Assets/tiny.png")));
    textureMap.insert(TileTex(TILE_ENEMY, nullptr));
    textureMap.insert(TileTex(TILE_CHEST, nullptr));
    textureMap.insert(TileTex(TILE_CONNECTOR, rm->LoadTexture("Assets/connector.png")));
}