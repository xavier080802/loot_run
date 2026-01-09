#ifndef _GAME_STATE_H
#define _GAME_STATE_H
#include "../game_state_manager.h"

class GameState : public State {
public:
	void InitState() override;
	void EnterState() override;
	void ExitState() override;
	void UnloadState() override;
	void Update(float dt) override; 

	GameState();
private:

};
#endif // !_GAME_STATE_H
