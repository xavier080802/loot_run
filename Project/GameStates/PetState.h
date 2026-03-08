#ifndef _PET_STATE_H_
#define _PET_STATE_H_

#include "../GameStateManager.h"
#include "AEEngine.h"

class PetState : public State {
public:
	void LoadState() override;
	void InitState() override;
	void ExitState() override;
	void UnloadState() override;
	void Update(double dt) override;
	void Draw() override;

private:
	void RenderPetInventory();
	void HandleInput();
	float scrollOffset{ 0.0f };
};

#endif // !_PET_STATE_H_