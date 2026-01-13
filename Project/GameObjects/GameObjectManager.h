#ifndef _GAME_OBJECT_MANAGER_H_
#define _GAME_OBJECT_MANAGER_H_
#include "AEEngine.h"
#include "../DesignPatterns/singleton.h"
#include <vector>

class GameObject;

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
	//Note: no idea what im doing, might be terribly optimized.
	struct SpatialGrid {
		//Init a grid of _numDimensions x _numDimensions to cover the world
		void Init(unsigned _worldWidth, unsigned _worldHeight, unsigned _numDimensions);
		void SortObjects(GameObject** gos, size_t count);
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
//NOTE: 13/1: Assuming only 1 state has game objects, otherwise need to implement something to prevent updating objs in other scenes.
class GameObjectManager : public Singleton<GameObjectManager>{
	friend Singleton<GameObjectManager>;
public:
	void RegisterGO(GameObject* go);
	//Note: Called by State
	void UpdateObjects(double dt);
	//Note: Called by State
	void DrawObjects();
	//Ensure large enough to cover whole playing area, otherwise the object wont have collision
	void InitCollisionGrid(unsigned width, unsigned height);

	GameObject* Clone(GameObject* const original);

private:
	//z sorted in ascending order
	std::vector<GameObject*> goList;

	GOCollision::SpatialGrid grid;

	~GameObjectManager();
};

#endif // !_GAME_OBJECT_MANAGER_H_
