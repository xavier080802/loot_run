#ifndef _GAME_OBJECT_MANAGER_H_
#define _GAME_OBJECT_MANAGER_H_
#include "AEEngine.h"
#include "../DesignPatterns/singleton.h"
#include "GOTypeList.h"
#include <vector>
#include <queue>

/*  GameObjectManager

	When a gameobject is created, it registers to the manager, which will handle its memory and lifecycle.

	Manager calls the GO's Key lifecycle functions like rendering, updating, and collision.

	A spatial grid is used to attempt to optimize collision between GOs.
	GOs collide with a tilemap too.
*/

class GameObject; //Avoid circular dependency
class TileMap;

namespace GOCollision {
	//Representation of an object
	struct Element {
		unsigned index; //index of the GO in its list
	};

	//A grid cell. Contains objects (elements)
	struct Cell {
		std::vector<Element> elements;
	};

	//Place objects into grid cells based on their position.
	//Check collision only between objects in the same cell
	struct SpatialGrid {
		//Init a grid of _numDimensions x _numDimensions to cover the world
		void Init(unsigned _worldWidth, unsigned _worldHeight, unsigned _numDimensions);
		void SortObjects(GameObject** gos, size_t count);
		//Get grid cell based on world position
		//x = column index, y = row index
		AEVec2 GetCell(AEVec2 const& pos) const;
		void InsertToCell(GameObject* go, unsigned ind, unsigned cellInd);

		unsigned partitions=0;
		unsigned cellWidth{}, cellHeight{}, worldHalfWidth{}, worldhalfHeight{};
		std::vector<Cell> cells;
	};
}

//********************************************************************************
//Singleton to manage game objects.
//Manages Update, Drawing, and collision.
//Currently needs the State to manually call Update/Draw to control the order.
//NOTE: 16/2: In GameStateManager, all GOs are disabled before the next state's Init.
//GOs should be Init in InitState which enabled them, hopefully preventing some weird behavior if there are multiple game states that need GOs.
class GameObjectManager : public Singleton<GameObjectManager>{
	friend Singleton<GameObjectManager>;
public:
	void RegisterGO(GameObject* go);
	//Note: Called by State
	void UpdateObjects(double dt, TileMap const* tilemap);
	//Note: Called by State
	void DrawObjects();
	//Ensure large enough to cover whole playing area, otherwise the object wont have collision
	void InitCollisionGrid(unsigned width, unsigned height);
	//Disable all gameobjects
	void DisableAllGOs();
	//Finds the first gameobject hovered by the mouse (world space)
	//Nullptr if nothing is found
	GameObject* QueryOnMouse();
	//Find the closest GO of the given type within range
	//Note: currently does not take into account the size of the other GOs
	GameObject* FindClosestGO(AEVec2 pos, float range, GO_TYPE type);

	//TODO: redo wtih copy swap idiom, and proper copy ctor
	GameObject* Clone(GameObject* const original);

	//Fetches an inactive gameobject of the given type, and enables it.
	//If no existing go is found, creates a new one. Returns nullptr if type is invalid/unimplemented
	//Always init the go returned.
	GameObject* FetchGO(GO_TYPE type);

	std::vector<GameObject*> const& GetGameObjects() const { return goList; }

private:
	//z sorted in ascending order
	std::vector<GameObject*> goList;
	std::queue<GameObject*> goRegistrationQ;

	GOCollision::SpatialGrid grid;

	bool isLoopingThrList{ false };

	//Response when GO collides with a tile - Prevent GO from moving into the tile.
	AEVec2 Helper_HandleGOTileCollision(AEVec2 tileInd, AEVec2 startPos, GameObject const& go, TileMap const& tilemap);

	~GameObjectManager();
};

#endif // !_GAME_OBJECT_MANAGER_H_
