#ifndef _MAIN_MENU_STATE_H
#define _MAIN_MENU_STATE_H
#include "../GameStateManager.h"

//The main menu
class MainMenuState : public State {
public:
	void LoadState() override;
	void InitState() override;
	void ExitState() override;
	void UnloadState() override;
	void Update(double dt) override; 
	void Draw() override;

private:

};
#endif // !_MAIN_MENU_STATE_H
