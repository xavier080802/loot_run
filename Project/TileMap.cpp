#include "TileMap.h"
#include "File/CSV.h"
#include "RenderingManager.h"
#include "Helpers/MatrixUtils.h"
#include "Helpers/RenderUtils.h"
#include "Helpers/CoordUtils.h"
#include "Helpers/Vec2Utils.h"
#include "camera.h"

//Init static map
std::map<TileMap::TILE_TYPE, AEGfxTexture*> TileMap::textureMap{};
std::map<TileMap::TILE_TYPE, TileMap::Tile> TileMap::tileMap{};
AEGfxVertexList* TileMap::pMesh{};

//Shortcut type for loading tex map
using TileTex = std::pair<TileMap::TILE_TYPE, AEGfxTexture*>;
using TilePair = std::pair<TileMap::TILE_TYPE, TileMap::Tile>;

TileMap::TileMap(std::string filename, AEVec2 offset, float tileX, float tileY)
	: tileSize{AEVec2{tileX, tileY}}, posOffset(offset)
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

		//For each column (AKA a value)
		for (unsigned c{}; c < cols; ++c) {
			std::string data{ file.GetData(r, c) };
			if (data == "") { //Empty/invalid
				tiles[r].push_back(tileMap[TILE_NONE]);
				continue;
			}
			//Parse to int. Crashes if value if invalid.
			int v = std::stoi(data);
			//If v is invalid, default to NONE
			tiles[r].push_back(tileMap[(v >= TILE_NUM ? TILE_NONE : static_cast<TILE_TYPE>(v))]); //Add column
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
			//NOTE: Rotation renders ACW
			DrawTintedMesh(GetTransformMtx(pos, rot, scale),
				pMesh, textureMap.at(tiles[r][c].type),
				{255,255,255,255}, 255);
		}
	}
}

void TileMap::Render(AEVec2 offsetPos,float rotOffset, AEVec2 scale, bool isHud) const
{
	for (unsigned int r = 0; r < rows; r++) {
		for (unsigned int c = 0; c < cols; c++) {
			if (tiles[r][c].type == TILE_NONE) continue;

			f32 rot = rotOffset;
			AEVec2 pos{ GetTilePosition(r,c) * scale }; //Change pos based on new tile size
			AEVec2 tileScale = tileSize;
			if (!isHud) GetObjViewFromCamera(&pos, &rot, &tileScale);
			DrawTintedMesh(GetTransformMtx(pos + offsetPos, rot, tileScale * scale),
				pMesh, textureMap.at(tiles[r][c].type),
				{ 255,255,255,255 }, 255);
		}
	}
}

AEVec2 TileMap::GetTilePosition(unsigned rowInd, unsigned colInd) const
{
	//Normally, Bottom left of map is at origin.
	AEVec2 out{
		colInd * tileSize.x + tileSize.x * 0.5f,
		(rows - 1 - rowInd) * tileSize.y
	};
	//Offset by -map_half_size to center map on origin. Then add posOffset
	return out - mapSize * 0.5f + posOffset;
}

AEVec2 TileMap::GetTileIndFromPos(AEVec2 pos) const
{
	AEVec2 p{ pos + mapSize * 0.5f - posOffset };
	AEVec2 out{
		(int)(p.x / tileSize.x),
		(int)(-(p.y - tileSize.y * 0.5f) / tileSize.y + (rows-1))
	};
	return out;
}

TileMap::Tile const* TileMap::QueryTile(AEVec2 pos) const
{
	AEVec2 inds{ GetTileIndFromPos(pos) };
	if (inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) return nullptr;
	return &tiles[inds.y][inds.x];
}

TileMap::Tile const* TileMap::QueryTile(unsigned rowInd, unsigned colInd) const
{
	if (colInd < 0 || colInd >= cols || rowInd < 0 || rowInd >= rows) return nullptr;
	return &tiles[rowInd][colInd];
}

std::pair<TileMap::Tile const*, AEVec2> TileMap::QueryTileAndInd(AEVec2 pos) const
{
	AEVec2 inds{ GetTileIndFromPos(pos) };
	//If Index out of bounds, tile should be nullptr
	return std::make_pair((inds.x < 0 || inds.x >= cols || inds.y < 0 || inds.y >= rows) ? nullptr : &tiles[inds.y][inds.x], inds);
}

TileMap::Tile const* TileMap::ChangeTile(unsigned row, unsigned col, TILE_TYPE newType)
{
	if (col < 0 || col >= cols || row < 0 || row >= rows) return nullptr;
	tiles[row][col] = tileMap[newType];
	return &tiles[row][col];
}

void TileMap::LoadStatics()
{
	//Load textures (once each per tile type)
	RenderingManager* rm = RenderingManager::GetInstance(); //Use to load texture
	pMesh = rm->GetMesh(MESH_SQUARE);

	//=======================Tile map========================

	//By default, all tiles are obstacles.
	for (int i{}; i < TILE_NUM; ++i) {
		tileMap.insert(TilePair(static_cast<TILE_TYPE>(i), { static_cast<TILE_TYPE>(i), Collision::OBSTACLE }));
	}
	//Exceptions
	tileMap[TILE_NONE].layer = Collision::NONE;
	tileMap[TILE_NONE].isSolid = false;

	//======================Texture map======================

	textureMap.insert(TileTex(TILE_NONE, nullptr));
	textureMap.insert(TileTex(TILE_WALL, rm->LoadTexture("Assets/finn.png")));
	textureMap.insert(TileTex(TILE_DOOR, rm->LoadTexture("Assets/tiny.png")));
}
