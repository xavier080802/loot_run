#ifndef PATHFINDER_H_
#define PATHFINDER_H_

#include "../GameStates/Related/TileMap.h"
#include "AEEngine.h"
#include <vector>
#include <deque>
#include <array>

/*	Pathfinder

	Class to handle Best-First pathfinding based on a tilemap.
	
	USAGE 
		Add as a parent class.
		Call UpdatePathfinding in an Update function,
		and DoPathFinding whenever (in Update is fine too)
*/
class Pathfinder
{
public:
	enum class RESULT {
		FAILED,
		SUCCESS,
		COOLDOWN,
	};
	struct Node {
		AEVec2 pos;
		unsigned id, parent, cost;

		Node(AEVec2 _nodePos, AEVec2 dest, unsigned _parentId, unsigned _id);
	};
	//Return the path found, if any
	std::deque<AEVec2> const& GetFoundPath() const;
	//Reset the pathfinder, removing the path and allowing it to immediately path-find again.
	void ResetPathfinder();

	//Set time (in seconds) before path refreshes
	void SetPathRefreshTime(float _time) { pathFindCooldown = _time; }
	//Draw nodes in the path
	void DrawPath();

protected:
	//Call in update.
	//Updates the timer
	void UpdatePathfinding(float dt);
	//Performs pathfinding.
	//Has a timer built-in for optimization. Call ResetPathfinder if calling this out of update.
	//Returns whether a path was made or not (No path found, or still on cooldown)
	RESULT DoPathFinding(TileMap const& map, AEVec2 const& start, AEVec2 const& dest);
	//Remove the first node in the path (if possible)
	void PopPath();
	//Add new pos to the path (pushed to back)
	void PushPath(AEVec2 const& newNodePos);

private:
	//Number of directions to look at for neighbours
	enum {DIRS=8};
	//Array of tile index offsets for the neighbours.
	const std::array<AEVec2, DIRS> Directions{
		AEVec2{-1, 1}, //Top-left
		AEVec2{0,1}, //Top
		AEVec2{1, 1}, //Top-right
		AEVec2{-1,0}, //Left
		AEVec2{1,0}, //Right
		AEVec2{-1, -1},//Bottom-left
		AEVec2{0,-1}, //Bottom
		AEVec2{1, -1}, //Bottom-right
	};

	//Return array of nodes surrounding the curr node
	//Invalid neighbours are not added
	std::vector<AEVec2> FindNeighbourNodes(Node const& curr, TileMap const& map) const;
	//True if the pos exists in the closed list.
	//Pos is tile pos (snapped to center)
	bool FindInClosedList(AEVec2 const& pos);
	//True if the pos exists in the open list
	//Pos is tile pos (snapped to center)
	bool FindInOpenList(AEVec2 const& pos);
	//To be called after DoPathfinding
	//Forms the actual path from the closed list
	void FormPath(Node const& last, AEVec2 dest);

	std::deque<Node> openList{};
	std::vector<Node> closedList{};
	std::deque<AEVec2> path{};
	//Optimization: Prevent pathfinding every frame
	float pathfindTimer{}, pathFindCooldown{ 1.f };
	//Keep track of Node IDs. 0 is basically "null"
	unsigned nextId{ 0 };
};

#endif // PATHFINDER_H_

