#ifndef _CREDIT_STATE_H
#define _CREDIT_STATE_H
#include "GameStateManager.h"
// The credits screen
class CreditState : public State {
public:
	void LoadState()       override;
	void InitState()       override;
	void ExitState()       override;
	void UnloadState()     override;
	void Update(double dt) override;
	void Draw()            override;
private:
};
#endif 
