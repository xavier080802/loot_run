#include "GameObjectManager.h"
#include "GameObject.h"
#include "../helpers/CollisionUtils.h"
#include "Projectile.h"
#include "LootChest.h"
#include "../Drops/PickupGO.h"
#include "AttackHitboxGO.h"
#include "../TileMap.h"
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/RenderUtils.h"
#include <iostream>

void GameObjectManager::RegisterGO(GameObject* go)
{
	//Modifying the goList while it's looping will explode.
	if (isLoopingThrList) {
		goRegistrationQ.push(go);
		return;
	}

	//Place go based on z. Ascending order.
	for (std::vector<GameObject*>::iterator it = goList.begin(); it != goList.end(); it++) {
		GameObject* curr = *it;
		if (go == curr) {
			//Dupe
			return;
		}
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
		go->Update(dt);
	}
	//Collision
	grid.SortObjects(goList.data(), goList.size());

	//Collide with tilemap
	for (GameObject* go : goList) {
		if (!go->collisionEnabled || !go->isEnabled) continue;

		//Hotspots at 45 deg angle intervals (0->360, 8 total)
		//Allow all hotspots to check in 1 frame (Do not break the loop)
		for (int h{}; h < 8;++h) {
			AEVec2 hotspotDir{ AECosDeg(h * 45), AESinDeg(h * 45) };
			//Check if hotspot collides with a tile by checking if hotspot is on a collidable tile.
			std::pair<TileMap::Tile const*, AEVec2> tile{ tilemap->QueryTileAndInd(go->pos + hotspotDir * go->scale * 0.5f) };
			if (!tile.first || !BitmaskContainsFlag(go->collisionLayers, tile.first->layer)) {
				continue; //Hotspot no collision detected.
			}
			//Position snapback to not overlap tile.
			if (tile.first->isSolid) Helper_HandleGOTileCollision(tile.second, *go, *tilemap);

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
	grid.Init(width, height, 5);
}

void GameObjectManager::DisableAllGOs()
{
	for (GameObject* go : goList) {
		go->SetEnabled(false);
	}
}

GameObject* GameObjectManager::Clone(GameObject* const original)
{
	GameObject* newGo = new GameObject(*original);
	//Re-init pointers (otherwise they point to the original's members, and this will crash when original deletes its memory)
	newGo->renderingData = new AnimationData(*original->renderingData);
	newGo->renderingData->texList = new AEGfxTexture* {*original->renderingData->texList};
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
	default:
		std::cout << "WARNING: FetchGO implementation not done for GO_TYPE " << (int)type << '\n';
		return nullptr;
	}
}

//Prevent clipping with solid tile
void GameObjectManager::Helper_HandleGOTileCollision(AEVec2 tileInd, GameObject& go, TileMap const& tilemap)
{
	//Collided with tile. Prevent clipping.
	AEVec2 tilePos{ tilemap.GetTilePosition(tileInd.y, tileInd.x) };
	//Get pos of the edge closest to the go.
	AEVec2 closest{ AEClamp(go.pos.x, tilePos.x - tilemap.GetTileSize().x * 0.5f, tilePos.x + tilemap.GetTileSize().x * 0.5f),
				AEClamp(go.pos.y, tilePos.y - tilemap.GetTileSize().y * 0.5f, tilePos.y + tilemap.GetTileSize().y * 0.5f) };
	//Distance between go and the closest edge.
	closest = go.pos - closest;
	//Overlap amount (how much the go is currently inside the tile): Radius - dist of go from edge
	float lenClos = AEVec2Length(&closest); 
	AEVec2 pen = { go.scale.x * 0.5f - lenClos, go.scale.y * 0.5f - lenClos };
	//Normalize to get dir to closest edge.
	AEVec2 closestNorm{};
	if (closest.x || closest.y) AEVec2Normalize(&closestNorm, &closest);
	//Push the GO away along the overlap direction.
	go.pos += closestNorm * pen;
}

GameObjectManager::~GameObjectManager()
{
	//Free and delete GOs
	for (GameObject* go : goList) {
		if (!go) continue;
		go->Free();
		delete go;
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
		int cellX = static_cast<int>((pos.x + worldHalfWidth) / (float)cellWidth);
		int cellY = static_cast<int>(std::abs(pos.y - worldhalfHeight) / (float)cellHeight);

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

void GOCollision::SpatialGrid::InsertToCell(GameObject* go, unsigned ind, unsigned cellInd)
{
	if (cellInd >= cells.size()) return;
	Element el{ ind };
	cells[cellInd].elements.push_back(el);
	go->cellIndexes.push_back(cellInd);
}
