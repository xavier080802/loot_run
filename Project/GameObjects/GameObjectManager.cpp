#include "GameObjectManager.h"
#include "GameObject.h"

//Assuming that GO->Init is called once in the GO's lifetime (so not checking for dupe)
void GameObjectManager::RegisterGO(GameObject* go)
{
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

void GameObjectManager::UpdateObjects(double dt)
{
	for (GameObject* go : goList) {
		if (!go->isEnabled) continue;
		go->Update(dt);
		//TODO: Collision
	}
}

void GameObjectManager::DrawObjects()
{
	for (GameObject* go : goList) {
		if (!go->isEnabled) continue;
		go->Draw();
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
