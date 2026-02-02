#include "../Project/Shop.h"
#include "../Project/Gacha.h"
#include "../Project/game_state_manager.h"
#include "AEEngine.h"
#include "Music.h"

static GachaAnimation gStateAnim;
static s8 gachaFont = -1;

void GachaState::LoadState() {
    gachaFont = AEGfxCreateFont(
        "Assets/Exo2/Exo2-SemiBoldItalic.ttf",
        32
    );

    EnsureOverlayMesh();
}

void GachaState::InitState() {
    bgm.StopGameplayBGM(); 
    bgm.PlayGacha();
    BeginGachaOverlay(gStateAnim, 10, 0.6f, 1.2f, 0.3f);
}

void GachaState::Update(double dt) {
    bool openPressed = AEInputCheckTriggered(AEVK_O) || AEInputCheckTriggered(0x4F);
    bool skipPressed = AEInputCheckTriggered(AEVK_SPACE);
    bool pull10 = AEInputCheckTriggered(AEVK_R) || AEInputCheckTriggered(0x52); 
    bool pull100 = AEInputCheckTriggered(AEVK_T) || AEInputCheckTriggered(0x54);
    bool exitPressed = AEInputCheckTriggered(AEVK_F) || AEInputCheckTriggered(AEVK_ESCAPE);

    if (exitPressed) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
        return;
    }

    if (gStateAnim.phase == GachaPhase::Done) {
        if (pull10) {
            BeginGachaOverlay(gStateAnim, 10, 0.1f, 0.8f, 0.3f);
        }
        else if (pull100) {
            BeginGachaOverlay(gStateAnim, 100, 0.1f, 1.2f, 0.2f);
        }
    }

    UpdateGachaOverlay(
        gStateAnim,
        static_cast<float>(dt),
        skipPressed,
        openPressed
    );

    if (gStateAnim.isFinished) {
        gStateAnim.phase = GachaPhase::Done;
    }
}

void GachaState::Draw() {
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    DrawGachaOverlay(gStateAnim, gachaFont);
}

void GachaState::ExitState() {
    bgm.StopGacha(0.2f);

    gStateAnim.Reset();
}

void GachaState::UnloadState() {
    gachaFont = -1;
}
