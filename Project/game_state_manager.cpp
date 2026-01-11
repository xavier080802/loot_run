#include "game_state_manager.h"
#include "AEEngine.h"

void GameStateManager::AddGameState(std::string stateName, State* state)
{
	if (stateList.find(stateName) != stateList.end()) //Prevent dupe
		return;

	stateList.insert(std::pair<std::string, State*>(stateName, state));
	state->InitState();
}

bool GameStateManager::SetNextGameState(std::string nextName)
{
	if (currState.first == nextName || stateList.find(nextName) == stateList.end()) {
		return false;
	}
	//Exit prev state
	if (currState.second) {
		stateList.find(currState.first)->second->ExitState();
	}
	//Enter new state
	currState.first = nextName;
	currState.second = stateList.find(nextName)->second;
	currState.second->EnterState();
	return true;
}

void GameStateManager::UpdateCurrState(double dt)
{
	if (!currState.second) return;
	currState.second->Update(dt);
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
