#ifndef _GAME_STATE_H
#define _GAME_STATE_H
#include "../game_state_manager.h"

//State for the game scene
class GameState : public State {
public:
	void InitState() override;
	void EnterState() override;
	void ExitState() override;
	void UnloadState() override;
	void Update(double dt) override; 

private:

};
#endif // !_GAME_STATE_H
