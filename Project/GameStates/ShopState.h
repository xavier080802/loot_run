#ifndef SHOPSTATE_H_
#define SHOPSTATE_H_

#include "GameStateManager.h"

//The shop menu
class ShopState : public State {
public:
	void LoadState() override;
	void InitState() override;
	void ExitState() override;
	void UnloadState() override;
	void Update(double dt) override; 
	void Draw() override;

private:

};

#endif // SHOPSTATE_H_

