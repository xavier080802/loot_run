#include "Pathfinder.h"
#include "../Helpers/Vec2Utils.h"
#include "../Rendering/RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../Rendering/Camera.h"
#include <algorithm>
#include "../DebugTools.h"

Pathfinder::RESULT Pathfinder::DoPathFinding(TileMap const& map, AEVec2 const& start, AEVec2 const& dest)
{
    if (pathfindTimer < pathFindCooldown) return RESULT::COOLDOWN;

    ResetPathfinder();
    //Check if start and dest are valid.
    if (!map.QueryTile(start) || !map.QueryTile(dest)) return RESULT::FAILED;
    pathfindTimer = 0.f; //After reset to set pathfinding on cooldown
    //Pos of the dest tile
    AEVec2 startTile{ map.SnapPosToTile(start) };
    AEVec2 destTile{ map.SnapPosToTile(dest) };
    openList.push_back(Node{ startTile, destTile,0, ++nextId });

    //Try to find path
    while (!openList.empty()) {
        Node curr{ openList.front()};
        openList.pop_front();
        closedList.push_back(curr);
        //Cost = 0 means reached destTile. Successful pathfinding.
        if (curr.cost == 0) {
            FormPath(curr, dest); //Form the usuable path
            return RESULT::SUCCESS;
        }

        //Find neighbour nodes
        std::vector<AEVec2> neighbours{ FindNeighbourNodes(curr, map)};
        for (AEVec2 const& np : neighbours) {
            //Find in list
            if (FindInClosedList(np) || FindInOpenList(np)) continue;
            //Add to open list
            openList.push_back(Node{ np, destTile, curr.id, ++nextId });
        }

        //Sort open list, so that lower cost is at the front
        std::sort(openList.begin(), openList.end(), [](Node const& a, Node const& b) {
            return a.cost < b.cost;
        });
    }
    return RESULT::FAILED;
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
    s8 font{ RenderingManager::GetInstance()->GetFont() };

    int i{};
    for (AEVec2 const& n : path) {
        f32 _rot = 0;
        AEVec2 _pos = n;
        AEVec2 _scale = { 10,10 };
        GetObjViewFromCamera(&_pos, &_rot, &_scale);
        DrawMeshWithTexOffset(GetTransformMtx(_pos, _rot, _scale),
            mesh,
            nullptr,
            i ? Color{255.f, 255.f /i, 255.f / i, 255} : Color{0, 255,0, 255}, 255,
            {0,0});

        DrawAEText(font, std::string{ std::to_string((int)n.x) +"\n"+ std::to_string((int)n.y)},
            { _pos.x, _pos.y - 10 }, 0.2f, 0.f, Color{ 255,255,10, 255 }, TEXT_ORIGIN_POS::TEXT_UPPER_MIDDLE);
        ++i;
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

std::vector<AEVec2> Pathfinder::FindNeighbourNodes(Node const& curr, TileMap const& map) const
{
    std::vector<AEVec2> neighbours{ };
    AEVec2 ind = map.GetTileIndFromPos(curr.pos);
    //Check adjacent tile in each cardinal direction
    for (size_t i{}; i < Directions.size(); ++i) {
        int nX{ static_cast<int>(ind.x + Directions[i].x) }, nY{ static_cast<int>(ind.y + Directions[i].y) };
        //Check if walkable and exists
        TileMap::Tile const* tile{ map.QueryTile(nY, nX) };
        if (!tile || tile->layer == Collision::LAYER::OBSTACLE) continue;

        //If diagonal, must check surrounding tiles - Prevent moving through 2 walls touching only at the corners
        if (Directions[i].x * Directions[i].y != 0) { //Diagonals x and y are both non-0
            //Surrounding cells are [x,0] and [0,y]
            TileMap::Tile const* s1{ map.QueryTile(0, nX) };
            TileMap::Tile const* s2{ map.QueryTile(nY, 0) };
            //If s1/s2 is nullptr, or both are obstacle, reject this diagonal.
            if (!s1 || !s2
                || (s1->layer == Collision::LAYER::OBSTACLE && s2->layer == Collision::LAYER::OBSTACLE)) continue;
        }
        neighbours.push_back(map.GetTilePosition(nY, nX));
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
