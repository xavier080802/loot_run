#ifndef _GAME_STATE_H
#define _GAME_STATE_H
#include "../GameStateManager.h"
#include "AEEngine.h"

extern std::string mapSelected;

//State for the game scene
class GameState : public State {
public:
	void LoadState() override;
	void InitState() override;
	void ExitState() override;
	void UnloadState() override;
	void Update(double dt) override; 
	void Draw() override;



private:
void HandleTutorialDialogueRender();
int   mLastMinuteMark = 0;
float mNotificationTimer = 0.0f;
};
bool getBossAlive();
float getBossHPProgressBar();
void setBossHPProgressBar(float current);
float getBossMaxHPProgressBar();
void setBossMaxHPProgressBar(float max);
#endif // !_GAME_STATE_H
