#include "Pathfinder.h"
#include "../Helpers/Vec2Utils.h"
#include "../RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../camera.h"
#include <algorithm>

bool Pathfinder::DoPathFinding(TileMap const& map, AEVec2 const& start, AEVec2 const& dest)
{
    if (pathfindTimer < pathFindCooldown) return false;

    ResetPathfinder();
    //Check if start and dest are valid.
    if (!map.QueryTile(start) || !map.QueryTile(dest)) return false;
    pathfindTimer = 0.f; //After reset to set pathfinding on cooldown
    //Pos of the dest tile
    AEVec2 startTile{ map.SnapPosToTile(start) };
    AEVec2 destTile{ map.SnapPosToTile(dest) };
    openList.push_back(Node{ startTile, destTile,0, ++nextId });

    //Try to find path
    while (!openList.empty()) {
        Node curr{ openList.front()};
        openList.pop_front();
        //Cost = 0 means reached destTile. Successful pathfinding.
        if (curr.cost == 0) {
            FormPath(curr, dest); //Form the usuable path
            return true;
        }

        closedList.push_back(curr);

        //Find neighbour nodes
        std::array<AEVec2, DIRS> neighbours{ FindNeighbourNodes(curr, map)};
        for (AEVec2 const& np : neighbours) {
            //Find in list
            if (FindInClosedList(np) || FindInOpenList(np)) continue;
            //Check if walkable and exists
            TileMap::Tile const* tile{map.QueryTile(np)};
            if (!tile || tile->layer == Collision::LAYER::OBSTACLE) continue;
            //Add to open list
            openList.push_back(Node{ np, destTile, curr.id, ++nextId });
        }

        //Sort open list, so that lower cost is at the front
        std::sort(openList.begin(), openList.end(), [](Node const& a, Node const& b) {
            return a.cost < b.cost;
        });
    }
    return false;
}

std::deque<AEVec2> const & Pathfinder::GetFoundPath() const
{
    return path;
}

void Pathfinder::ResetPathfinder() {
    closedList.clear();
    openList.clear();
    path.clear();
    nextId = 0;
    pathfindTimer = pathFindCooldown;
}

void Pathfinder::DrawPath()
{
    if (path.empty()) return;

    AEGfxVertexList* mesh{ RenderingManager::GetInstance()->GetMesh(MESH_CIRCLE) };

    float c{ 255.f / (path.size()) };
    for (AEVec2 const& n : path) {
        f32 _rot = 0;
        AEVec2 _pos = n;
        AEVec2 _scale = {20,20};
        GetObjViewFromCamera(&_pos, &_rot, &_scale);
        DrawMeshWithTexOffset(GetTransformMtx(_pos, _rot, _scale),
            mesh,
            nullptr,
            {c,255,c,255}, 255,
            {0,0});

        c += c;
    }
}

void Pathfinder::UpdatePathfinding(float dt)
{
    if (pathfindTimer < pathFindCooldown)
        pathfindTimer += dt;
}

void Pathfinder::PopPath()
{
    if (path.empty()) return;
    path.pop_front();
}

void Pathfinder::PushPath(AEVec2 const& newNodePos)
{
    path.push_back(newNodePos);
}

std::array<AEVec2, Pathfinder::DIRS> Pathfinder::FindNeighbourNodes(Node const& curr, TileMap const& map) const
{
    std::array<AEVec2, DIRS> neighbours{};
    AEVec2 ind = map.GetTileIndFromPos(curr.pos);
    //Check adjacent tile in each cardinal direction
    for (size_t i{}; i < Directions.size(); ++i) {
        neighbours[i] = map.GetTilePosition(ind.y + Directions[i].y, ind.x + Directions[i].x);
    }
    return neighbours;
}

bool Pathfinder::FindInClosedList(AEVec2 const& pos)
{
    for (Node const& n: closedList) {
        if (n.pos == pos) return true;
    }
    return false;
}

bool Pathfinder::FindInOpenList(AEVec2 const& pos)
{
    for (Node const& n : openList) {
        if (n.pos == pos) return true;
    }
    return false;
}

void Pathfinder::FormPath(Node const& last, AEVec2 dest)
{
    if (closedList.empty()) return;

    //Push back exact dest pos, so that object will move off tile grid directly to destination
    path.push_back(dest);
    Node step{ last };
    //Search from last node backwards through parent id
    //Until parent id hits 0 (the first node)
    while (step.parent) {
        path.push_front(step.pos);
        //Find parent node
        for (Node const& n : closedList) {
            if (n.id != step.parent) continue;
            step = n;
            break;
        }
    }
}

//Node ctor
Pathfinder::Node::Node(AEVec2 _nodePos, AEVec2 dest, unsigned _parentId, unsigned _id)
    : pos{_nodePos}, cost{(unsigned)AEVec2Distance(&dest, &_nodePos)}, parent{_parentId}, id{_id}
{
}
