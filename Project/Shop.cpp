#include "../Project/Shop.h"
#include "../Project/Gacha.h"
#include "../Project/game_state_manager.h"
#include "AEEngine.h"

static GachaAnimation gStateAnim;
static s8 gachaFont = -1;

void GachaState::LoadState() {
    gachaFont = AEGfxCreateFont("Assets/Exo2/Exo2-SemiBoldItalic.ttf", 32);
    EnsureOverlayMesh();
}

void GachaState::InitState() {
    BeginGachaOverlay(gStateAnim, 10, 0.6f, 1.2f, 0.3f);
}

void GachaState::Update(double dt) {

    // Key inputs
    bool openPressed = AEInputCheckTriggered(AEVK_O) || AEInputCheckTriggered(0x4F); // 'O'
    bool rawSkipPressed = AEInputCheckTriggered(AEVK_SPACE);

    // Only allow skipping during Reveal/Done (prevents skipping the chest phase)
    bool skipPressed =
        (gStateAnim.phase == GachaPhase::Reveal || gStateAnim.phase == GachaPhase::Done)
        ? rawSkipPressed
        : false;

    bool exitPressed = AEInputCheckTriggered(AEVK_F);

    bool pull10 = AEInputCheckTriggered(0x52) || AEInputCheckTriggered(AEVK_R);
    bool pull100 = AEInputCheckTriggered(0x54) || AEInputCheckTriggered(AEVK_T);

    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
        return;
    }

    // ? Call ONCE (this is the only call you need)
    UpdateGachaOverlay(gStateAnim, (float)dt, skipPressed, openPressed);

    // If animation completes, keep it in Done
    if (gStateAnim.isFinished) {
        gStateAnim.phase = GachaPhase::Done;
    }

    // Post-roll options
    if (gStateAnim.phase == GachaPhase::Done) {
        if (pull10) {
            BeginGachaOverlay(gStateAnim, 10, 0.1f, 0.8f, 0.2f);
        }
        else if (pull100) {
            BeginGachaOverlay(gStateAnim, 100, 0.1f, 1.2f, 0.015f);
        }
        else if (exitPressed) {
            GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
        }
    }
}

void GachaState::Draw() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    DrawGachaOverlay(gStateAnim, gachaFont);
}

void GachaState::ExitState() {
    gStateAnim.Reset();
}

void GachaState::UnloadState() {
    gachaFont = -1;
}