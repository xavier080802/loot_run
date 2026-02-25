#ifndef _SHOP_STATE_H
#define _Shop_STATE_H
#include "../GameStateManager.h"

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
#endif // !_SHOP_STATE_H
