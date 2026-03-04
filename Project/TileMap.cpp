#include "TileMap.h"
#include "File/CSV.h"
#include "RenderingManager.h"
#include "Helpers/MatrixUtils.h"
#include "Helpers/RenderUtils.h"
#include "Helpers/CoordUtils.h"
#include "Helpers/Vec2Utils.h"
#include "camera.h"
#include <cstdlib> // For srand and rand

//Init static map
std::map<TileMap::TILE_TYPE, AEGfxTexture*> TileMap::textureMap{};
std::map<TileMap::TILE_TYPE, TileMap::Tile> TileMap::tileMap{};
AEGfxVertexList* TileMap::pMesh{};

//Shortcut type for loading tex map
using TileTex = std::pair<TileMap::TILE_TYPE, AEGfxTexture*>;
using TilePair = std::pair<TileMap::TILE_TYPE, TileMap::Tile>;

TileMap::TileMap(std::string filename, AEVec2 offset, float tileX, float tileY)
	: tileSize{ AEVec2{tileX, tileY} }, posOffset(offset)
{
	//If static map isnt loaded, load it.
	if (!textureMap.size()) {
		LoadStatics();
	}

	CSV file{ filename };
	rows = file.GetRows();
	cols = file.GetCols();
	mapSize = { cols * tileX, rows * tileY };
	tiles.reserve(rows);
	//Populate tilemap
	for (unsigned r{}; r < rows; ++r) {
		//Create row of tiles.
		tiles.push_back({});
		tiles[r].reserve(cols);

		for (unsigned c{}; c < cols; ++c) {
			std::string data{ file.GetData(r, c) };
			if (data == "") {
				tiles[r].push_back(tileMap[TILE_NONE]);
				continue;
			}
			int v = std::stoi(data);
			tiles[r].push_back(tileMap[(v >= TILE_NUM ? TILE_NONE : static_cast<TILE_TYPE>(v))]); //Add column
		}
	}
}

//Proccedural constructor 
TileMap::TileMap(AEVec2 offset, float tileX, float tileY)
	: tileSize{ AEVec2{tileX, tileY} }, posOffset(offset), rows{ 0 }, cols{ 0 }
{
	if (!textureMap.size()) {
		LoadStatics();
	}
}

// Logic for proccedural
void TileMap::GenerateProcedural(unsigned int r, unsigned int c, int seed)
{
	rows = r;
	cols = c;
	mapSize = { cols * tileSize.x, rows * tileSize.y };

	// Initialize random with the seed
	srand(seed);

	// Fill with walls initially (The solid '1's in your image)
	tiles.clear();
	tiles.resize(rows, std::vector<Tile>(cols, tileMap[TILE_WALL]));

	for (unsigned int i = 1; i < rows - 1; ++i) {
		for (unsigned int j = 1; j < cols - 1; ++j) {

			if ((rand() % 100) < 40) {
				tiles[i][j] = tileMap[TILE_NONE];
			}
		}
	}

	int midR = rows / 2;
	int midC = cols / 2;

	// LEFT ENTRANCE (Use Connector instead of None)
	tiles[midR][0] = tileMap[TILE_CONNECTOR]; // Changed from TILE_NONE
	tiles[midR][1] = tileMap[TILE_CONNECTOR]; // Changed from TILE_NONE

	// RIGHT EXIT
	tiles[midR][cols - 1] = tileMap[TILE_CONNECTOR]; // Changed from TILE_NONE
	tiles[midR][cols - 2] = tileMap[TILE_CONNECTOR]; // Changed from TILE_NONE

	// TOP/BOTTOM CONNECTORS
	tiles[0][midC] = tileMap[TILE_CONNECTOR]; // Changed from TILE_NONE
	tiles[1][midC] = tileMap[TILE_CONNECTOR]; // Changed from TILE_NONE
	tiles[rows - 1][midC] = tileMap[TILE_CONNECTOR]; // Changed from TILE_NONE
	tiles[rows - 2][midC] = tileMap[TILE_CONNECTOR]; // Changed from TILE_NONE

	// Decoration Pass
	for (unsigned int i = 1; i < rows - 1; ++i) {
		for (unsigned int j = 1; j < cols - 1; ++j) {
			if (tiles[i][j].type == TILE_NONE) {
				if (i == midR || j == midC) continue;

				int chance = rand() % 100;
				if (chance < 2) tiles[i][j] = tileMap[TILE_ENEMY];
				else if (chance < 3) tiles[i][j] = tileMap[TILE_CHEST];
			}
		}
	}
}

void TileMap::Render() const
{
	for (unsigned int r = 0; r < rows; r++) {
		for (unsigned int c = 0; c < cols; c++) {
			if (tiles[r][c].type == TILE_NONE) continue;

			f32 rot = 0;
			AEVec2 pos{ GetTilePosition(r,c) };
			AEVec2 scale = tileSize;
			GetObjViewFromCamera(&pos, &rot, &scale);
			DrawTintedMesh(GetTransformMtx(pos, rot, scale),
				pMesh, textureMap.at(tiles[r][c].type),
				{ 255,255,255,255 }, 255);
		}
	}
}

void TileMap::Render(AEVec2 offsetPos, float rotOffset, AEVec2 scale, bool isHud) const
{
	for (unsigned int r = 0; r < rows; r++) {
		for (unsigned int c = 0; c < cols; c++) {
			if (tiles[r][c].type == TILE_NONE) continue;
			auto it = textureMap.find(tiles[r][c].type);
			if (it == textureMap.end() || it->second == nullptr) {
				continue;
			}

			f32 rot = rotOffset;
			AEVec2 pos{ GetTilePosition(r,c) * scale };
			AEVec2 tileScale = tileSize;

			if (!isHud) GetObjViewFromCamera(&pos, &rot, &tileScale);
			DrawTintedMesh(GetTransformMtx(pos + offsetPos, rot, tileScale * scale),
				pMesh, it->second,
				{ 255,255,255,255 }, 255);
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
	return std::make_pair((inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) ? nullptr : &tiles[(unsigned)inds.y][(unsigned)inds.x], inds);
}

TileMap::Tile const* TileMap::ChangeTile(unsigned row, unsigned col, TILE_TYPE newType)
{
	if (col < 0 || col >= cols || row < 0 || row >= rows) return nullptr;
	tiles[row][col] = tileMap[newType];
	return &tiles[row][col];
}

bool TileMap::IsWall(AEVec2 worldPos) const {
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

AEVec2 TileMap::GetSecondRowSpawn() const
{
	if (rows < 2) return { 0,0 };

	for (unsigned col = 0; col < cols; ++col)
	{
		if (tiles[1][col].type == TILE_NONE)
			return GetTilePosition(1, col);
	}

	return GetTilePosition(1, 0);
}

// Added implementation to fix LNK2019 unresolved external symbol error
AEVec2 TileMap::GetSpawnPoint() const
{
	for (unsigned int r = 0; r < rows; ++r) {
		for (unsigned int c = 0; c < cols; ++c) {
			if (tiles[r][c].type == TILE_NONE || tiles[r][c].type == TILE_CONNECTOR) {
				return GetTilePosition(r, c);
			}
		}
	}
	return posOffset;
}

void TileMap::LoadStatics()
{
	//Load textures 
	RenderingManager* rm = RenderingManager::GetInstance();
	pMesh = rm->GetMesh(MESH_SQUARE);
	//By default, all tiles are obstacles.
	for (int i{}; i < TILE_NUM; ++i) {
		tileMap.insert(TilePair(static_cast<TILE_TYPE>(i), { static_cast<TILE_TYPE>(i), Collision::OBSTACLE }));
	}
	//Exceptions
	tileMap[TILE_NONE].layer = Collision::NONE;
	tileMap[TILE_NONE].isSolid = false;

	tileMap[TILE_ENEMY].layer = Collision::NONE;
	tileMap[TILE_ENEMY].isSolid = false;

	tileMap[TILE_CHEST].layer = Collision::NONE;
	tileMap[TILE_CHEST].isSolid = false;

	tileMap[TILE_CONNECTOR].layer = Collision::NONE;
	tileMap[TILE_CONNECTOR].isSolid = false;

	textureMap.insert(TileTex(TILE_NONE, nullptr));
	textureMap.insert(TileTex(TILE_WALL, rm->LoadTexture("Assets/finn.png")));
	textureMap.insert(TileTex(TILE_DOOR, rm->LoadTexture("Assets/tiny.png")));
	textureMap.insert(TileTex(TILE_ENEMY, rm->LoadTexture("Assets/enemyplaceholder.png")));
	textureMap.insert(TileTex(TILE_CHEST, rm->LoadTexture("Assets/chestplaceholder.png")));
	textureMap.insert(TileTex(TILE_CONNECTOR, rm->LoadTexture("Assets/connector.png")));
}