#ifndef MAINMENUSTATE_H_
#define MAINMENUSTATE_H_

#include "GameStateManager.h"

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

#endif // MAINMENUSTATE_H_

