#ifndef _GAME_STATE_MANAGER_H_
#define _GAME_STATE_MANAGER_H_
#include "./DesignPatterns/singleton.h"
#include <string>
#include <map>

//Abstract class. Each game state to inherit these functions
class State {
public:
	//Load asset resources
	virtual void LoadState() = 0;
	//Initialize state to starting values / Clean slate
	virtual void InitState() = 0;
	//Update state logic
	virtual void Update(double dt) = 0;
	//Draw state
	virtual void Draw() = 0;
	//Exits state without freeing assets
	virtual void ExitState() = 0;
	//Frees resources from memory (if any). Signifies the end of this state.
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
	void DrawCurrState();

private:
	std::pair<std::string, State*> currState;
	std::map<std::string, State*> stateList;

	~GameStateManager();
};

#endif // !_GAME_STATE_MANAGER_H_
