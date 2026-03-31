#include "GameStateManager.h"
#include "AEEngine.h"
#include "GameObjects/GameObjectManager.h"

void GameStateManager::AddGameState(std::string stateName, State* state)
{
	if (stateList.find(stateName) != stateList.end()) //Prevent dupe
		return;

	stateList.insert(std::pair<std::string, State*>(stateName, state));
	state->LoadState();
}

bool GameStateManager::SetNextGameState(std::string nextName, bool initNext, bool exitCurr, bool disableGOs)
{
	if (currState.first == nextName || stateList.find(nextName) == stateList.end()) {
		return false;
	}
	//Exit prev state
	if (currState.second && exitCurr) {
		stateList.find(currState.first)->second->ExitState();
	}
	prevState = currState;
	if (disableGOs) {
		//Disable all gameobjects in manager. Let InitState reenable needed GOs
		GameObjectManager::GetInstance()->DisableAllGOs();
	}
	//Enter new state
	currState.first = nextName;
	currState.second = stateList.find(nextName)->second;
	if (initNext) currState.second->InitState();
	return true;
}

bool GameStateManager::ReturnToPrevState(bool reInitPrev, bool exitCurr)
{
	if (!prevState.second)
		return false;

	return SetNextGameState(prevState.first, reInitPrev, exitCurr, false);
}

void GameStateManager::UpdateCurrState(double dt)
{
	if (!currState.second) return;
	currState.second->Update(dt);
}

void GameStateManager::DrawCurrState()
{
	if (!currState.second) return;
	currState.second->Draw();
}

GameStateManager::~GameStateManager()
{
	//Delete states
	for (std::map<std::string, State*>::iterator it = stateList.begin(); it != stateList.end(); it++) {
		if (!it->second) continue;
		it->second->UnloadState();
		delete it->second;
	}
	stateList.clear();
}
