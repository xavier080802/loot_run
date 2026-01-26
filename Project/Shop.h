#ifndef SHOP_STATE_H
#define SHOP_STATE_H

#include "../Project/game_state_manager.h"
#include "AEEngine.h"

// The Gacha Scene State
class GachaState : public State {
public:
    void LoadState() override;
    void InitState() override;
    void Update(double dt) override;
    void Draw() override;
    void ExitState() override;
    void UnloadState() override;
};

#endif
