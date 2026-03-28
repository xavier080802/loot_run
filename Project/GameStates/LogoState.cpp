#include "LogoState.h"
#include "../RenderingManager.h"

void LogoState::LoadState() {
    pLogoTex = AEGfxTextureLoad("Assets/digipen.png");
}

void LogoState::InitState() {
    elapsed = 0.0f;
}

void LogoState::Update(double dt) {
    elapsed += (float)dt;

    if (elapsed > FADE_DURATION + HOLD_TIME + FADE_OUT_DURATION) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
    }

    if (AEInputCheckTriggered(AEVK_SPACE) || AEInputCheckTriggered(AEVK_RETURN)) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
    }
}

void LogoState::Draw() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    if (!pLogoTex) return;

    float alpha = 0.0f;

    if (elapsed < FADE_DURATION) {
        alpha = elapsed / FADE_DURATION;
    }
    else if (elapsed < FADE_DURATION + HOLD_TIME) {
        alpha = 1.0f;
    }
    else if (elapsed < FADE_DURATION + HOLD_TIME + FADE_OUT_DURATION) {
        float t = (elapsed - FADE_DURATION - HOLD_TIME) / FADE_OUT_DURATION;
        alpha = 1.0f - t;
    }
    else {
        alpha = 0.0f;
    }

    float winW = (float)(AEGfxGetWinMaxX() - AEGfxGetWinMinX());
    float drawW = winW * 0.6f;
    float aspect = 445.0f / 1525.0f;
    float drawH = drawW * aspect;

    AEMtx33 scale, trans, final;
    AEMtx33Scale(&scale, drawW, drawH);
    AEMtx33Trans(&trans, 0.0f, 0.0f);
    AEMtx33Concat(&final, &trans, &scale);
    AEGfxSetTransform(final.m);

    AEGfxTextureSet(pLogoTex, 0, 0);
    AEGfxSetTransparency(alpha);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    AEGfxMeshDraw(
        RenderingManager::GetInstance()->GetMesh(MESH_SQUARE),
        AE_GFX_MDM_TRIANGLES
    );
}

void LogoState::ExitState() {}

void LogoState::UnloadState() {
    if (pLogoTex) {
        AEGfxTextureUnload(pLogoTex);
        pLogoTex = nullptr;
    }
}