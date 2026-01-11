#ifndef _GAME_OBJECT_MANAGER_H_
#define _GAME_OBJECT_MANAGER_H_
#include "AEEngine.h"
#include "../DesignPatterns/singleton.h"
#include <vector>

class GameObject;

class GameObjectManager : public Singleton<GameObjectManager>{
	friend Singleton<GameObjectManager>;
public:
	void RegisterGO(GameObject* go);
	void UpdateObjects(double dt);
	void DrawObjects();

	GameObject* Clone(GameObject* const original);

private:
	//z sorted in ascending order
	std::vector<GameObject*> goList;

	~GameObjectManager();
};

#endif // !_GAME_OBJECT_MANAGER_H_
