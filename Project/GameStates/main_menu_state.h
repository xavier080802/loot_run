#ifndef _MAIN_MENU_STATE_H
#define _MAIN_MENU_STATE_H
#include "../game_state_manager.h"

//The main menu
class MainMenuState : public State {
public:
	void InitState() override;
	void EnterState() override;
	void ExitState() override;
	void UnloadState() override;
	void Update(double dt) override; 

private:

};
#endif // !_MAIN_MENU_STATE_H
