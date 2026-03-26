#include "LogoState.h"
#include "../RenderingManager.h"

// -------- Timing --------
static const float FLASH_TIME = 0.15f;      // black ? white flash
static const float FADE_DURATION = 1.0f;    // logo fade in
static const float HOLD_TIME = 1.0f;        // stay visible
static const float FADE_OUT_DURATION = 0.5f;

void LogoState::LoadState() {
    pLogoTex = AEGfxTextureLoad("Assets/DigiPen_BLACK.png");
}

void LogoState::InitState() {
    elapsed = 0.0f;
}

void LogoState::Update(double dt) {
    elapsed += (float)dt;

    // Full sequence timing
    float totalTime = FLASH_TIME + FADE_DURATION + HOLD_TIME + FADE_OUT_DURATION;

    if (elapsed > totalTime) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
    }

    // Skip
    if (AEInputCheckTriggered(AEVK_SPACE) || AEInputCheckTriggered(AEVK_RETURN)) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
    }
}

void LogoState::Draw() {
    float totalFlashEnd = FLASH_TIME;
    float fadeInEnd = totalFlashEnd + FADE_DURATION;
    float holdEnd = fadeInEnd + HOLD_TIME;
    float fadeOutEnd = holdEnd + FADE_OUT_DURATION;

    // -------- Background Flash --------
    if (elapsed < FLASH_TIME * 0.5f) {
        // Start BLACK
        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    }
    else if (elapsed < FLASH_TIME) {
        // WHITE flash
        AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);
    }
    else {
        // Stay WHITE
        AEGfxSetBackgroundColor(1.0f, 1.0f, 1.0f);
    }

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    // -------- Logo Alpha --------
    float alpha = 0.0f;

    if (elapsed < totalFlashEnd) {
        alpha = 0.0f;
    }
    else if (elapsed < fadeInEnd) {
        float t = (elapsed - totalFlashEnd) / FADE_DURATION;
        alpha = t * t; // ease-in
    }
    else if (elapsed < holdEnd) {
        alpha = 1.0f;
    }
    else if (elapsed < fadeOutEnd) {
        float t = (elapsed - holdEnd) / FADE_OUT_DURATION;
        alpha = 1.0f - t;
    }
    else {
        alpha = 0.0f;
    }

    // -------- Logo Size --------
    float winW = (float)AEGfxGetWinMaxX() - AEGfxGetWinMinX();
    float drawW = winW * 0.6f;

    float aspect = 445.0f / 1525.0f;
    float drawH = drawW * aspect;

    // -------- Transform --------
    AEMtx33 scale, trans, final;

    AEMtx33Scale(&scale, drawW, drawH);
    AEMtx33Trans(&trans, 0.0f, 0.0f);

    AEMtx33Concat(&final, &trans, &scale);

    AEGfxSetTransform(final.m);

    // -------- Draw Logo --------
    if (alpha > 0.0f && pLogoTex) {
        AEGfxTextureSet(pLogoTex, 0, 0);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, alpha);

        AEGfxMeshDraw(
            RenderingManager::GetInstance()->GetMesh(MESH_SQUARE),
            AE_GFX_MDM_TRIANGLES
        );
    }
}

void LogoState::ExitState() {
    // Nothing needed
}

void LogoState::UnloadState() {
    if (pLogoTex) {
        AEGfxTextureUnload(pLogoTex);
        pLogoTex = nullptr;
    }
}