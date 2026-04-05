#ifndef LEVELSELECTSTATE_H_
#define LEVELSELECTSTATE_H_

#include "GameStateManager.h"

extern std::string mapSelected;

//Level Selection state
class LevelSelectState : public State {
public:
	void LoadState() override;
	void InitState() override;
	void ExitState() override;
	void UnloadState() override;
	void Update(double dt) override;
	void Draw() override;

private:

};

#endif // LEVELSELECTSTATE_H_

