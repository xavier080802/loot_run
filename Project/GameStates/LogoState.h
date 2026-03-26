
#ifndef LOGO_H
#define LOGO_H
#include "AEEngine.h"
#include "../GameStateManager.h" 

class LogoState : public State
{
public:
    virtual void LoadState() override;
    virtual void InitState() override;
    virtual void Update(double dt) override;
    virtual void Draw() override;
    virtual void ExitState() override;
    virtual void UnloadState() override;

private:
    AEGfxTexture* pLogoTex;
    float elapsed;
    const float FADE_DURATION = 3.0f;
};

#endif

