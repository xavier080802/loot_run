#ifndef _TILE_MAP_H_
#define _TILE_MAP_H_
#include <string>
#include <map>
#include <vector>
#include "AEEngine.h"
#include "GameObjects/GameObject.h"

class TileMap
{
public:
	enum TILE_TYPE {
		TILE_NONE = 0,
		TILE_WALL,

		TILE_NUM, //Last
	};

	//A tile on the map
	//Using struct if we want to add A star pathfinding later.
	struct Tile {
		TILE_TYPE type;
		GameObject::COLLISION_LAYER layer;
	};

	//Note: If a value in csv is invalid, defaults to TILE_NONE.
	//offset: Positional offset from origin. Center of map is {0,0} by default.
	TileMap(std::string filename, AEVec2 offset = {0,0}, float tileX = 25.f, float tileY = 25.f);

	void Render() const;
	//NOTE: y is row, x is col
	AEVec2 GetTilePosition(unsigned rowInd, unsigned colInd) const;
	//Calculates where the pos will be in the tilemap (in index). Return values are rounded to whole numbers
	AEVec2 GetTileIndFromPos(AEVec2 pos) const;
	//Get the tile type at the given pos.
	Tile const* QueryTile(AEVec2 pos) const;
	//NOTE: y is row, x is col
	Tile const* QueryTile(unsigned rowInd, unsigned colInd) const;

	AEVec2 GetTileSize() const { return tileSize; }

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
