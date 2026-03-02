#ifndef _TILE_MAP_H_
#define _TILE_MAP_H_
#include <string>
#include <map>
#include <vector>
#include "AEEngine.h"
#include "CollisionConstants.h"

class TileMap
{
public:
	enum TILE_TYPE {
		TILE_NONE = 0,    // 0: Floor
		TILE_WALL,        // 1: Solid Wall
		TILE_DOOR,        // 2: The Exit Door (Room 6)
		TILE_ENEMY,       // 3: Enemy Spawn Point
		TILE_CHEST,       // 4: Loot Chest

		TILE_NUM, // Total: 5
	};

	//A tile on the map
	struct Tile {
		TILE_TYPE type;
		Collision::LAYER layer;
		bool isSolid; //Whether can pass through (while still triggering OnCollideTile)

		Tile(TILE_TYPE _type = TILE_NONE, Collision::LAYER _layer = Collision::OBSTACLE, bool _isSolid = true)
			: type{ _type }, layer{ _layer }, isSolid{_isSolid}
		{}
	};

	//Note: If a value in csv is invalid, defaults to TILE_NONE.
	//offset: Positional offset from origin. Center of map is {0,0} by default.
	TileMap(std::string filename, AEVec2 offset = {0,0}, float tileX = 25.f, float tileY = 25.f);

	// Tilemap for procedural generated map
	TileMap(AEVec2 offset = { 0,0 }, float tileX = 25.f, float tileY = 25.f);
	//void GenerateProcedural(unsigned int r, unsigned int c, int maxFloorTiles);
	//AEVec2 GetSpawnPoint() const;

	void Render() const;
	void Render(AEVec2 offsetPos, float rotOffset, AEVec2 scale, bool isHud) const;
	//NOTE: y is row, x is col. Gets the center of the tile at the indices
	AEVec2 GetTilePosition(unsigned rowInd, unsigned colInd) const;
	//Calculates where the pos will be in the tilemap (in index). Return values are rounded to whole numbers
	AEVec2 GetTileIndFromPos(AEVec2 pos) const;
	//Get the tile type at the given pos.
	Tile const* QueryTile(AEVec2 pos) const;
	//NOTE: y is row, x is col
	Tile const* QueryTile(unsigned rowInd, unsigned colInd) const;
	//Get the Tile, and its Index. Tile will be nullptr if the pos is out of bounds.
	//First: Pointer to the tile at the pos
	//Second: Index of the tile
	std::pair<Tile const*, AEVec2> QueryTileAndInd(AEVec2 pos) const;

	AEVec2 GetTileSize() const { return tileSize; }
	//Get Cols(first) and Rows(second) of the map
	std::pair<unsigned, unsigned> GetMapSize() const { return std::make_pair(cols, rows); };
	AEVec2 GetFullMapSize() const { return { tileSize.x * cols, tileSize.y * rows }; }

	//Change tile type. Returns the tile
	Tile const* ChangeTile(unsigned row, unsigned col, TILE_TYPE newType);

	std::vector<std::vector<Tile>> const& GetTileData() const { return tiles; }

private:
	static void LoadStatics();
	//Tile data
	static std::map<TILE_TYPE, Tile> tileMap;
	//Tile texture data
	static std::map<TILE_TYPE, AEGfxTexture*> textureMap;
	//Mesh to draw tiles with.
	static AEGfxVertexList* pMesh; 
	
	std::vector<std::vector<Tile>> tiles{};
	unsigned rows{}, cols{};
	AEVec2 tileSize{};
	//Offset from origin
	AEVec2 posOffset{};
	//Total map size calculated after csv is read
	AEVec2 mapSize{};
};
#endif // !_TILE_MAP_H_
