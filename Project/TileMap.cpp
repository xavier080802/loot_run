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

using TileTex = std::pair<TileMap::TILE_TYPE, AEGfxTexture*>;
using TilePair = std::pair<TileMap::TILE_TYPE, TileMap::Tile>;

TileMap::TileMap(std::string filename, AEVec2 offset, float tileX, float tileY)
    : tileSize{ AEVec2{tileX, tileY} }, posOffset(offset)
{
    if (!textureMap.size()) LoadStatics();

    CSV file{ filename };
    rows = file.GetRows();
    cols = file.GetCols();
    mapSize = { cols * tileX, rows * tileY };

    tiles.reserve(rows);
    for (unsigned r{}; r < rows; ++r) {
        tiles.push_back({});
        tiles[r].reserve(cols);
        for (unsigned c{}; c < cols; ++c) {
            std::string data{ file.GetData(r, c) };
            if (data == "") { tiles[r].push_back(tileMap[TILE_NONE]); continue; }
            int v = std::stoi(data);
            tiles[r].push_back(tileMap[(v >= TILE_NUM ? TILE_NONE : static_cast<TILE_TYPE>(v))]);
        }
    }
}

TileMap::TileMap(AEVec2 offset, float tileX, float tileY)
    : tileSize{ AEVec2{tileX, tileY} }, posOffset(offset), rows{ 0 }, cols{ 0 }
{
    if (!textureMap.size()) LoadStatics();
}

// ============================================================
//  HasClearance
//  Returns true only when EVERY tile within 'radius' steps of
//  (row, col) exists and is non-solid.  Out-of-bounds counts
//  as solid so spawn points can never be placed near the edge.
// ============================================================
bool TileMap::HasClearance(unsigned row, unsigned col, int radius) const
{
    for (int dr = -radius; dr <= radius; ++dr) {
        for (int dc = -radius; dc <= radius; ++dc) {
            int r = (int)row + dr;
            int c = (int)col + dc;
            // Treat anything outside the map as solid
            if (r < 0 || r >= (int)rows || c < 0 || c >= (int)cols) return false;
            if (tiles[r][c].isSolid) return false;
        }
    }
    return true;
}

// ============================================================
//  GenerateProcedural
// ============================================================
void TileMap::GenerateProcedural(unsigned int r, unsigned int c, int seed)
{
    rows = r;
    cols = c;
    mapSize = { cols * tileSize.x, rows * tileSize.y };

    srand(seed);

    tiles.clear();
    tiles.resize(rows, std::vector<Tile>(cols, tileMap[TILE_WALL]));

    // --- Step 1: Random fill ---
    for (unsigned int i = 1; i < rows - 1; ++i)
        for (unsigned int j = 1; j < cols - 1; ++j)
            tiles[i][j] = (rand() % 100 < 55) ? tileMap[TILE_NONE] : tileMap[TILE_WALL];

    // --- Step 2: Smooth (3 passes of cellular automata) ---
    for (int pass = 0; pass < 3; ++pass) {
        std::vector<std::vector<Tile>> next = tiles;
        for (unsigned int i = 1; i < rows - 1; ++i) {
            for (unsigned int j = 1; j < cols - 1; ++j) {
                int wallCount = 0;
                for (int di = -1; di <= 1; ++di)
                    for (int dj = -1; dj <= 1; ++dj)
                        if (tiles[i + di][j + dj].type == TILE_WALL) ++wallCount;
                next[i][j] = (wallCount >= 5) ? tileMap[TILE_WALL] : tileMap[TILE_NONE];
            }
        }
        tiles = next;
    }

    int midR = rows / 2;
    int midC = cols / 2;

    // --- Step 3: Guaranteed 5x5 open centre ---
    for (int dr = -2; dr <= 2; ++dr)
        for (int dc = -2; dc <= 2; ++dc) {
            int rr = midR + dr, cc = midC + dc;
            if (rr > 0 && rr < (int)rows - 1 && cc > 0 && cc < (int)cols - 1)
                tiles[rr][cc] = tileMap[TILE_NONE];
        }

    // --- Step 4: Cross corridors ---
    for (unsigned int j = 1; j < cols - 1; ++j)
        if (tiles[midR][j].type == TILE_WALL) tiles[midR][j] = tileMap[TILE_NONE];
    for (unsigned int i = 1; i < rows - 1; ++i)
        if (tiles[i][midC].type == TILE_WALL) tiles[i][midC] = tileMap[TILE_NONE];

    // --- Step 5: Edge connectors ---
    tiles[midR][0] = tileMap[TILE_CONNECTOR];
    tiles[midR][1] = tileMap[TILE_CONNECTOR];
    tiles[midR][2] = tileMap[TILE_NONE];
    tiles[midR][cols - 1] = tileMap[TILE_CONNECTOR];
    tiles[midR][cols - 2] = tileMap[TILE_CONNECTOR];
    tiles[midR][cols - 3] = tileMap[TILE_NONE];
    tiles[0][midC] = tileMap[TILE_CONNECTOR];
    tiles[1][midC] = tileMap[TILE_CONNECTOR];
    tiles[2][midC] = tileMap[TILE_NONE];
    tiles[rows - 1][midC] = tileMap[TILE_CONNECTOR];
    tiles[rows - 2][midC] = tileMap[TILE_CONNECTOR];
    tiles[rows - 3][midC] = tileMap[TILE_NONE];

    // --- Step 6: Enemy and chest placement ---
    for (unsigned int i = 3; i < rows - 3; ++i) {
        for (unsigned int j = 3; j < cols - 3; ++j) {

            // FIX: Explicitly ensure we are only spawning on empty floor tiles
            if (tiles[i][j].type != TILE_NONE) continue;

            bool onCorridor = ((int)i == midR || (int)j == midC);
            int  dr = (int)i - midR, dc = (int)j - midC;
            bool inCenter = (dr >= -2 && dr <= 2 && dc >= -2 && dc <= 2);
            if (onCorridor || inCenter) continue;

            // Use HasClearance with radius 3 to ensure a 3x3 buffer of non-solid tiles
            if (!HasClearance(i, j, 3)) continue;

            int chance = rand() % 100;
            if (chance < 4) {
                tiles[i][j] = tileMap[TILE_ENEMY];
            }
            else if (chance < 7) {
                // By checking TILE_NONE above, we ensure we don't overwrite 
                // something already placed or spawn in a clumped wall area
                tiles[i][j] = tileMap[TILE_CHEST];
            }
        }
    }
}
// ============================================================
//  Render
// ============================================================
void TileMap::Render() const
{
    for (unsigned int r = 0; r < rows; r++) {
        for (unsigned int c = 0; c < cols; c++) {
            //if (tiles[r][c].type == TILE_NONE) continue;
            auto it = textureMap.find(tiles[r][c].type);
            if (it == textureMap.end() || it->second == nullptr) continue;

            f32    rot = 0;
            AEVec2 pos{ GetTilePosition(r, c) };
            AEVec2 scale = tileSize;
            GetObjViewFromCamera(&pos, &rot, &scale);
            DrawTintedMesh(GetTransformMtx(pos, rot, scale), pMesh, it->second, { 255,255,255,255 }, 255);
        }
    }
}

void TileMap::Render(AEVec2 offsetPos, float rotOffset, AEVec2 scale, bool isHud) const
{
    for (unsigned int r = 0; r < rows; r++) {
        for (unsigned int c = 0; c < cols; c++) {
            //if (tiles[r][c].type == TILE_NONE) continue;
            auto it = textureMap.find(tiles[r][c].type);
            if (it == textureMap.end() || it->second == nullptr) continue;

            f32    rot = rotOffset;
            AEVec2 pos{ GetTilePosition(r, c) * scale };
            AEVec2 tileScale = tileSize;
            if (!isHud) GetObjViewFromCamera(&pos, &rot, &tileScale);
            DrawTintedMesh(GetTransformMtx(pos + offsetPos, rot, tileScale * scale),
                pMesh, it->second, { 255,255,255,255 }, 255);
        }
    }
}

AEVec2 TileMap::GetTilePosition(unsigned rowInd, unsigned colInd) const
{
    AEVec2 out{
        colInd * tileSize.x + tileSize.x * 0.5f,
        (rows - 1 - rowInd) * tileSize.y
    };
    return out - mapSize * 0.5f + posOffset;
}

AEVec2 TileMap::GetTileIndFromPos(AEVec2 pos) const
{
    AEVec2 p{ pos + mapSize * 0.5f - posOffset };
    AEVec2 out{
        (f32)((int)(p.x / tileSize.x)),
        (f32)((int)(-(p.y - tileSize.y * 0.5f) / tileSize.y + (rows - 1)))
    };
    return out;
}

TileMap::Tile const* TileMap::QueryTile(AEVec2 pos) const
{
    AEVec2 inds{ GetTileIndFromPos(pos) };
    if (inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) return nullptr;
    return &tiles[(unsigned)inds.y][(unsigned)inds.x];
}

TileMap::Tile const* TileMap::QueryTile(unsigned rowInd, unsigned colInd) const
{
    if (colInd < 0 || colInd >= cols || rowInd < 0 || rowInd >= rows) return nullptr;
    return &tiles[rowInd][colInd];
}

std::pair<TileMap::Tile const*, AEVec2> TileMap::QueryTileAndInd(AEVec2 pos) const
{
    AEVec2 inds{ GetTileIndFromPos(pos) };
    return std::make_pair(
        (inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows)
        ? nullptr : &tiles[(unsigned)inds.y][(unsigned)inds.x],
        inds);
}

AEVec2 TileMap::SnapPosToTile(AEVec2 pos) const
{
    AEVec2 inds{ GetTileIndFromPos(pos) };
    return GetTilePosition((unsigned)inds.y, (unsigned)inds.x);
}

TileMap::Tile const* TileMap::ChangeTile(unsigned row, unsigned col, TILE_TYPE newType)
{
    if (col < 0 || col >= cols || row < 0 || row >= rows) return nullptr;
    tiles[row][col] = tileMap[newType];
    return &tiles[row][col];
}

bool TileMap::IsWall(AEVec2 worldPos) const
{
    AEVec2 inds = GetTileIndFromPos(worldPos);
    if (inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) return true;
    return tiles[(unsigned)inds.y][(unsigned)inds.x].type == TILE_WALL;
}

bool TileMap::IsConnector(AEVec2 worldPos) const
{
    AEVec2 inds = GetTileIndFromPos(worldPos);
    if (inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) return false;
    return tiles[(unsigned)inds.y][(unsigned)inds.x].type == TILE_CONNECTOR;
}

bool TileMap::IsDoor(AEVec2 worldPos) const
{
    AEVec2 inds = GetTileIndFromPos(worldPos);
    if (inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) return false;
    return tiles[(unsigned)inds.y][(unsigned)inds.x].type == TILE_DOOR;
}

AEVec2 TileMap::GetSecondRowSpawn() const
{
    if (rows < 2) return { 0, 0 };
    for (unsigned col = 0; col < cols; ++col)
        if (tiles[1][col].type == TILE_NONE) return GetTilePosition(1, col);
    return GetTilePosition(1, 0);
}

AEVec2 TileMap::GetSpawnPoint() const
{
    int midR = (int)rows / 2;
    int midC = (int)cols / 2;

    for (int radius = 0; radius < (int)rows; ++radius) {
        for (int dr = -radius; dr <= radius; ++dr) {
            for (int dc = -radius; dc <= radius; ++dc) {
                if (abs(dr) != radius && abs(dc) != radius) continue;
                int r = midR + dr;
                int c = midC + dc;
                if (r < 1 || r >= (int)rows - 1 || c < 1 || c >= (int)cols - 1) continue;
                if (tiles[r][c].type != TILE_NONE) continue;

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
    return posOffset;
}

void TileMap::LoadStatics()
{
    RenderingManager* rm = RenderingManager::GetInstance();
    pMesh = rm->GetMesh(MESH_SQUARE);

    for (int i{}; i < TILE_NUM; ++i)
        tileMap.insert(TilePair(static_cast<TILE_TYPE>(i),
            { static_cast<TILE_TYPE>(i), Collision::OBSTACLE }));

    tileMap[TILE_NONE].layer = Collision::NONE;
    tileMap[TILE_NONE].isSolid = false;
    tileMap[TILE_ENEMY].layer = Collision::NONE;
    tileMap[TILE_ENEMY].isSolid = false;
    tileMap[TILE_CHEST].layer = Collision::NONE;
    tileMap[TILE_CHEST].isSolid = false;
    tileMap[TILE_CONNECTOR].layer = Collision::NONE;
    tileMap[TILE_CONNECTOR].isSolid = false;
    tileMap[TILE_DOOR].layer = Collision::NONE;
    tileMap[TILE_DOOR].isSolid = false;
    tileMap[TILE_BOSS] .layer = Collision::NONE;
    tileMap[TILE_BOSS].isSolid = false;

    textureMap.insert(TileTex(TILE_NONE, rm->LoadTexture("Assets/sprites/tiles/floor.png")));
    textureMap.insert(TileTex(TILE_WALL, rm->LoadTexture("Assets/sprites/tiles/wall.png")));
    textureMap.insert(TileTex(TILE_DOOR, rm->LoadTexture("Assets/sprites/tiles/connector.png")));
    textureMap.insert(TileTex(TILE_ENEMY, rm->LoadTexture("Assets/sprites/tiles/floor.png")));
    textureMap.insert(TileTex(TILE_BOSS, rm->LoadTexture("Assets/sprites/tiles/floor.png")));
    textureMap.insert(TileTex(TILE_CHEST, rm->LoadTexture("Assets/sprites/tiles/floor.png")));
    textureMap.insert(TileTex(TILE_CONNECTOR, rm->LoadTexture("Assets/sprites/tiles/connector.png")));
}