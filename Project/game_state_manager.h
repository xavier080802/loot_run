#ifndef _GAME_STATE_MANAGER_H_
#define _GAME_STATE_MANAGER_H_
#include "./DesignPatterns/singleton.h"
#include <string>
#include <map>

//Abstract class. Each game state to inherit these functions
class State {
public:
	//Called by state manager.
	virtual void InitState() = 0;
	virtual void EnterState() = 0;
	virtual void Update(double dt) = 0;
	virtual void ExitState() = 0;
	virtual void UnloadState() = 0;

	State() = default;
	virtual ~State() {}
};

//Singleton to manage all game states
class GameStateManager : public Singleton<GameStateManager>
{
	friend Singleton<GameStateManager>;
public:
	void AddGameState(std::string stateName, State* state);
	bool SetNextGameState(std::string nextName);
	void UpdateCurrState(double dt);

private:
	std::pair<std::string, State*> currState;
	std::map<std::string, State*> stateList;

	~GameStateManager();
};

#endif // !_GAME_STATE_MANAGER_H_
