#include "GameObjectManager.h"
#include "GameObject.h"
#include "../helpers/CollisionUtils.h"
#include "../TileMap.h"
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/RenderUtils.h"
#include "Projectile.h"
#include "LootChest.h"
#include "../Drops/PickupGO.h"
#include "AttackHitboxGO.h"
#include "../Pets/Pet.h"
#include "../Pets/Pet_1.h"
#include "../Pets/Pet_2.h"
#include "../Pets/Pet_3.h"
#include "../Pets/Pet_4.h"
#include "../Pets/Pet_5.h"
#include "../Pets/Pet_6.h"
#include "../Pets/Whirlpool.h"
#include <iostream>
#include <limits>

void GameObjectManager::RegisterGO(GameObject* go)
{
	//Modifying the goList while it's looping will explode.
	if (isLoopingThrList) {
		goRegistrationQ.push(go);
		return;
	}
	//Check dupe and remove previous entry
	//Iterate through entire array to guarantee that dupes are removed.
	for (std::vector<GameObject*>::iterator it = goList.begin(); it != goList.end(); it++) {
		GameObject* curr = *it;
		if (go == curr) {
			//Allow it to re-add itself to the list.
			goList.erase(it);
			break;
		}
	}
	//Place go based on z. Ascending order.
	for (std::vector<GameObject*>::iterator it = goList.begin(); it != goList.end(); it++) {
		GameObject* curr = *it;
		if (go->z < curr->z) {
			goList.insert(it, go); //Inserts go before curr.
			return;
		}
	}
	//Highest z, add to end
	goList.push_back(go);
}

void GameObjectManager::UpdateObjects(double dt, TileMap const* tilemap)
{
	isLoopingThrList = true;
	for (GameObject* go : goList) {
		if (!go->isEnabled) continue;
		go->prevPos = go->pos;
		go->Update(dt);
	}
	//Collision
	grid.SortObjects(goList.data(), goList.size());

	//Collide with tilemap
	for (GameObject* go : goList) {
		if (!go->collisionEnabled || !go->isEnabled) continue;

		//Dist between prev and curr pos
		float sqrTravelDist{ AEVec2SquareDistance(&go->prevPos, &go->pos) };
		const float stepDist{ min(go->scale.x * 0.5f, 10.f) };
		//Change in pos is big, check collision via raycast-style
		if (sqrTravelDist >= stepDist * stepDist) {
			AEVec2 dir{ go->pos - go->prevPos };
			AEVec2Normalize(&dir, &dir);
			//Number of steps between prev and curr. Distance between steps is stepDist
			unsigned numSteps{ static_cast<unsigned>(sqrtf(sqrTravelDist) / stepDist) };
			for (unsigned i{}; i <= numSteps; ++i) {
				//Pos of this step
				AEVec2 newPos{ go->prevPos + dir * (stepDist * i) };
				bool collided{false};
				//Hotspots at 45 deg angle intervals (0->360, 8 total)
				//Allow all hotspots to check (Do not break the loop)
				for (int h{}; h < 8;++h) {
					AEVec2 hotspotDir{ AECosDeg(h * 45.f), AESinDeg(h * 45.f) };
					AEVec2 hsPos{ newPos + hotspotDir * go->scale * 0.5f };
					//Check if hotspot collides with a tile by checking if hotspot is on a collidable tile.
					std::pair<TileMap::Tile const*, AEVec2> tile{ tilemap->QueryTileAndInd(hsPos) };
					if (!tile.first || !BitmaskContainsFlag(go->collisionLayers, tile.first->layer)) {
						continue; //Hotspot no collision detected.
					}
					//Position snapback to not overlap tile.
					if (tile.first->isSolid) {
						AEVec2 temp = Helper_HandleGOTileCollision(tile.second, newPos, *go, *tilemap);
						//Set to true if there was overlap of > 0. Never set back to false
						collided = collided ? true : (newPos != temp);
						newPos = temp;
					}

					go->OnCollideTile(std::make_pair(*tile.first, tile.second));
				}
				//After checking hotspots. Set pos to the offset-position 
				if (collided) {
					go->pos = newPos;
					break; //Collision resolved this step
				}
			}
		}
		//Number of steps wont reach the curr pos,
		//the below code will check go->pos

		//Hotspots at 45 deg angle intervals (0->360, 8 total)
		//Allow all hotspots to check in 1 frame (Do not break the loop)
		for (int h{}; h < 8;++h) {
			AEVec2 hotspotDir{ AECosDeg(h * 45.f), AESinDeg(h * 45.f) };
			AEVec2 hsPos{ go->pos + hotspotDir * go->scale * 0.5f };
			//Check if hotspot collides with a tile by checking if hotspot is on a collidable tile.
			std::pair<TileMap::Tile const*, AEVec2> tile{ tilemap->QueryTileAndInd(hsPos) };
			if (!tile.first || !BitmaskContainsFlag(go->collisionLayers, tile.first->layer)) {
				continue; //Hotspot no collision detected.
			}
			//Position snapback to not overlap tile.
			if (tile.first->isSolid) {
				go->pos = Helper_HandleGOTileCollision(tile.second, go->pos, *go, *tilemap);
			}

			go->OnCollideTile(std::make_pair(*tile.first, tile.second));
		}
	}

	for (GameObject* go : goList) {
		//Only let "player" (Player, pets, player's projs) query grid
		if (go->colliderLayer != Collision::LAYER::PLAYER
			|| !go->isEnabled || !go->collisionEnabled) continue;
		//For each cell this go is in...
		for (unsigned cell : go->cellIndexes) {
			//Check against other GOs in the same cell.
			for (GOCollision::Element otherInd : grid.cells[cell].elements) {
				GameObject* other = goList[otherInd.index];
				//Check if "other" can be collided with.
				if (other == go || !other->collisionEnabled || !other->isEnabled
					|| !BitmaskContainsFlag(go->collisionLayers, other->colliderLayer)) continue;

				//Sent to go
				GameObject::CollisionData data{ *other };
				//Sent to other
				GameObject::CollisionData otherData{ *go };
				//Check collision based on collider type
				if (go->colShape == Collision::SHAPE::COL_CIRCLE) {
					if ((other->colShape == Collision::SHAPE::COL_CIRCLE && CircleCollision(go->pos, other->pos, go->colSize.x/2.f, other->colSize.x/2.f))
						|| (other->colShape == Collision::SHAPE::COL_RECT && CircleRectCollision(other->pos, other->colSize, go->pos, go->colSize.x/2.f))) {
						//Collided
						go->OnCollide(data);
						other->OnCollide(otherData);
					}
				}
				//go is a Rect
				else if ((other->colShape == Collision::SHAPE::COL_CIRCLE && CircleRectCollision(go->pos, go->colSize, other->pos, other->colSize.x/2.f))
					|| (other->colShape == Collision::SHAPE::COL_RECT && IsRectsOverlapping(go->pos,go->colSize.x, go->colSize.y, other->pos, other->colSize.x, other->colSize.y))) {
					//Collided
					go->OnCollide(data);
					other->OnCollide(otherData);
				}
				//Check if this go has been disabled in current collision(if any)
				if (!go->isEnabled) {
					break;
				}
			}
			//Check if this go has been disabled after checking this cell
			if (!go->isEnabled) {
				break;
			}
		}
	}
	isLoopingThrList = false;

	//Clear up the registration queue
	while (!goRegistrationQ.empty()) {
		RegisterGO(goRegistrationQ.front());
		goRegistrationQ.pop();
	}
}

void GameObjectManager::DrawObjects()
{
	for (GameObject* go : goList) {
		if (!go->isEnabled) continue;
		go->Draw();
	}
}

void GameObjectManager::InitCollisionGrid(unsigned width, unsigned height)
{
	grid.Init(width, height, 10);
}

void GameObjectManager::DisableAllGOs()
{
	for (GameObject* go : goList) {
		go->SetEnabled(false);
	}
}

GameObject* GameObjectManager::QueryOnMouse()
{
	//Find cell of mouse
	AEVec2 mouse{ GetMouseWorldVec() };
	AEVec2 inds{ grid.GetCell(mouse) };
	size_t i{ static_cast<size_t>(inds.y * grid.partitions + inds.x) };
	if (i >= grid.cells.size()) return nullptr; //Out of bounds
	//Find gameobject on mouse
	for (GOCollision::Element const& e : grid.cells.at(i).elements) {
		GameObject& go{ *goList.at(e.index) };
		if (!go.IsEnabled()) continue;
		if ((go.colShape == Collision::COL_RECT && IsCursorOverWorld(go.GetPos(), go.GetColSize(), false))
			|| (go.colShape == Collision::COL_CIRCLE && IsCursorOverOvalWorld(go.GetPos(), go.GetColSize(),0, false))) {
			//Collision
			return &go;
		}
	}
	return nullptr;
}

GameObject* GameObjectManager::FindClosestGO(AEVec2 pos, float range, GO_TYPE type)
{
	static const std::array<AEVec2, 8> Directions{
		AEVec2{-1, 1}, //Top-left
		AEVec2{0,1}, //Top
		AEVec2{1, 1}, //Top-right
		AEVec2{-1,0}, //Left
		AEVec2{1,0}, //Right
		AEVec2{-1, -1},//Bottom-left
		AEVec2{0,-1}, //Bottom
		AEVec2{1, -1}, //Bottom-right
	};
	//Cell of pos
	AEVec2 inds{ grid.GetCell(pos) };
	unsigned cellX{ (unsigned)inds.x };
	unsigned cellY{ (unsigned)inds.y };
	std::vector<unsigned> cells{};
	unsigned sz{ (unsigned)(std::ceilf(range / max(grid.cellHeight, grid.cellWidth)) + 1) }; //Should be min 2 for range < cell size
	unsigned numCells{ sz *sz};
	cells.reserve(numCells);
	cells.push_back(cellY * grid.partitions + cellX);

	for (unsigned i{1}; i <= sz; ++i) {
		for (size_t dir{}; dir < Directions.size(); ++dir) {
			unsigned _x{ cellX + (int)Directions[dir].x * sz };
			unsigned _y{ cellY + (int)Directions[dir].y * sz };
			if (_y * grid.partitions + _x >= grid.cells.size()) {
				continue;
			}
			cells.push_back(_y * grid.partitions + _x);
		}
	}
	//Check each cell
	GameObject* nearest = nullptr;
	float nearestDist{ };
	for (unsigned c : cells) {
		for (GOCollision::Element const& e : grid.cells.at(c).elements) {
			GameObject& go{ *goList.at(e.index) };
			if (go.GetGOType() != type) continue;

			AEVec2 goPos{ go.GetPos() };
			float dist{ AEVec2SquareDistance(&goPos, &pos) };
			if (dist > range * range) continue; //Out of range
			//Check if nearer than prev
			if (!nearest || dist < nearestDist) {
				nearest = &go;
				nearestDist = dist;
			}
		}
	}

	return nearest;
}

//BROKEN
GameObject* GameObjectManager::Clone(GameObject* const original)
{
	GameObject* newGo = new GameObject(*original);
	//Re-init pointers (otherwise they point to the original's members, and this will crash when original deletes its memory)
	newGo->renderingData = new AnimationData(*original->renderingData);
	//newGo->renderingData->texList = new AEGfxTexture* {*original->renderingData->texList};
	//Rebuild mesh
	newGo->renderingData->mesh = nullptr;
	newGo->renderingData->InitAnimation(original->renderingData->spriteSheetRows, original->renderingData->spriteSheetCols);
	
	original->CompleteClone(newGo);
	RegisterGO(newGo);
	return newGo;
}

GameObject* GameObjectManager::FetchGO(GO_TYPE type)
{
	//Find existing disabled
	for (GameObject* go : goList) {
		if (go->isEnabled || go->GetGOType() != type) continue;
		go->isEnabled = true;
		return go;
	}

	//Create new
	switch (type)
	{
	case GO_TYPE::NONE:
		return new GameObject{};
	case GO_TYPE::PROJECTILE:
		return new Projectile{};
	case GO_TYPE::LOOT_CHEST:
		return new LootChest{};
	case GO_TYPE::ITEM:
		return new PickupGO{};
	case GO_TYPE::ATTACK_HITBOX:
		return new AttackHitboxGO{};
	case GO_TYPE::PET_1:
		return new Pet_1{};
	case GO_TYPE::PET_2:
		return new Pet_2{};
	case GO_TYPE::PET_3:
		return new Pet_3{};
	case GO_TYPE::PET_4:
		return new Pet_4{};
	case GO_TYPE::PET_5:
		return new Pet_5{};
	case GO_TYPE::PET_6:
		return new Pet_6{};
	case GO_TYPE::WHIRLPOOL:
		return new Whirlpool{};
	default:
		std::cout << "WARNING: FetchGO implementation not done for GO_TYPE " << (int)type << '\n';
		return nullptr;
	}
}

//Prevent clipping with solid tile
AEVec2 GameObjectManager::Helper_HandleGOTileCollision(AEVec2 tileInd, AEVec2 startPos, GameObject const& go, TileMap const& tilemap)
{
	//Collided with tile. Prevent clipping.
	AEVec2 tilePos{ tilemap.GetTilePosition((unsigned)tileInd.y, (unsigned)tileInd.x) };
	const AEVec2 tileSize{ tilemap.GetTileSize() * 0.5f };

	//Get pos of the rect edge closest to the go
	AEVec2 closest{};
	closest.x = max(tilePos.x - tileSize.x, min(tilePos.x + tileSize.x, startPos.x));
	closest.y = max(tilePos.y - tileSize.y, min(tilePos.y + tileSize.y, startPos.y));
	AEVec2 closestDir{ closest - startPos };
	float l = AEVec2Length(&closestDir);
	float overlap{ go.scale.x * 0.5f - l };
	if (overlap <= 0.f) return startPos;
	if (closestDir.x || closestDir.y) closestDir = closestDir / l;
	//Offset by the penetration distance along the penetration direction
	return startPos - closestDir * overlap;
}

GameObjectManager::~GameObjectManager()
{
	//Free and delete GOs
	for (GameObject* go : goList) {
		if (!go) continue;
		go->Free();
		delete go;
		go = nullptr;
	}
	goList.clear();
}

void GOCollision::SpatialGrid::Init(unsigned _worldWidth, unsigned _worldHeight, unsigned _numCells)
{
	partitions = _numCells;
	worldHalfWidth = _worldWidth/2;
	worldhalfHeight = _worldHeight /2;
	cellWidth = _worldWidth / partitions;
	cellHeight = _worldHeight / partitions;
	cells.clear();
	//grid: partitions * partitions
	cells.insert(cells.begin(), partitions* partitions, {});
}

void GOCollision::SpatialGrid::SortObjects(GameObject** gos, size_t count)
{
	cells.clear();
	//grid: partitions * partitions
	cells.insert(cells.begin(), partitions * partitions, {});

	for (unsigned i = 0;i < count; i++) {
		if (!gos[i]->CanCollide()) continue;
		AEVec2 pos = gos[i]->GetPos();
		AEVec2 ind{ GetCell(pos) };
		int cellX = static_cast<int>(ind.x);
		int cellY = static_cast<int>(ind.y);

		gos[i]->cellIndexes.clear();

		//Add to cell
		InsertToCell(gos[i], i, cellY * partitions + cellX);

		//Check bounding box
		AEVec2 bounds = gos[i]->GetColSize();
		int minX = static_cast<int>(pos.x - bounds.x) + worldHalfWidth;
		int maxX = static_cast<int>(pos.x + bounds.x) + worldHalfWidth;
		int minY = static_cast<int>(pos.y - bounds.y) - worldhalfHeight;
		int maxY = static_cast<int>(pos.y + bounds.y) - worldhalfHeight;
		//If the obj is fat and crosses boundaries, add to the other cells
		if (minX < cellX * static_cast<int>(cellWidth)) {
			InsertToCell(gos[i], i, cellY * partitions + cellX - 1);
		}
		if (maxX > (cellX + 1) * static_cast<int>(cellWidth)) {
			InsertToCell(gos[i], i, cellY * partitions + cellX + 1);
		}
		if (minY < cellY * static_cast<int>(cellHeight)) {
			InsertToCell(gos[i], i, (cellY - 1) * partitions + cellX);
		}
		if (maxY > (cellY + 1) * static_cast<int>(cellHeight)) {
			InsertToCell(gos[i], i, (cellY + 1) * partitions + cellX);
		}
	}
}

AEVec2 GOCollision::SpatialGrid::GetCell(AEVec2 const& pos) const
{
	int cellX = static_cast<int>((pos.x + worldHalfWidth) / (float)cellWidth);
	int cellY = static_cast<int>(std::abs(pos.y - worldhalfHeight) / (float)cellHeight);
	return AEVec2{(float)cellX, (float)cellY};
}

void GOCollision::SpatialGrid::InsertToCell(GameObject* go, unsigned ind, unsigned cellInd)
{
	if (cellInd >= cells.size()) return;
	Element el{ ind };
	cells[cellInd].elements.push_back(el);
	go->cellIndexes.push_back(cellInd);
}
