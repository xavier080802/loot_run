#ifndef _GAME_STATE_MANAGER_H_
#define _GAME_STATE_MANAGER_H_
#include "./DesignPatterns/singleton.h"
#include <string>
#include <map>

//Abstract class. Each game state to override the base functions.
//LoadState: Allocate state resources
//InitState: Set state to default.
//Update: State logic
//Draw: State rendering (Note that GameObjects are rendered with GameObjectManager::GetInstance()->DrawObjects()
//ExitState: State will not be updated. Exit logic.
//UnloadState: State resources are cleaned up.
class State {
public:
	//Load asset resources. Is called when added to the manager.
	virtual void LoadState() = 0;
	//Initialize state to starting values / Clean slate
	virtual void InitState() = 0;
	//Update state logic
	virtual void Update(double dt) = 0;
	//Draw state
	virtual void Draw() = 0;
	//Exits state without freeing assets
	virtual void ExitState() = 0;
	//Frees resources from memory (if any). Signifies the end of this state. Called when manager is destroyed.
	virtual void UnloadState() = 0;

	State() = default;
	virtual ~State() {}
};

//************************************************************************
//Singleton to manage all game states
class GameStateManager : public Singleton<GameStateManager>
{
	friend Singleton<GameStateManager>;
public:
	void AddGameState(std::string stateName, State* state);
	/// <summary>
	/// Changes game state.
	/// </summary>
	/// <param name="nextName">Name of the next state to change to.</param>
	/// <param name="initNext">If true, the next state will have its Init called.</param>
	/// <param name="exitCurr">If true, the next state will have its Exit called</param>
	/// <returns>Whether the operation is successful/state exists</returns>
	bool SetNextGameState(std::string nextName, bool initNext, bool exitCurr);
	/// <summary>
	/// Changes the game state to the previous state (if any)
	/// </summary>
	/// <param name="reInitPrev">Whether to call prev state's Init</param>
	/// <param name="exitCurr">Whether to call current state's Exit</param>
	/// <returns>Whether the state exists/operation is successful</returns>
	bool ReturnToPrevState(bool reInitPrev, bool exitCurr);
	void UpdateCurrState(double dt);
	void DrawCurrState();

private:
	std::pair<std::string, State*> currState;
	std::pair<std::string, State*> prevState;
	std::map<std::string, State*> stateList;

	~GameStateManager();
};

#endif // !_GAME_STATE_MANAGER_H_
