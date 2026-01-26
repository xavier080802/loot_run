#include "../Project/Shop.h"
#include "../Project/Gacha.h"
#include "../Project/game_state_manager.h"

static GachaAnimation gStateAnim;
static s8 gachaFont = -1;

void GachaState::LoadState() {
    gachaFont = AEGfxCreateFont("Assets/Exo2/Exo2-SemiBoldItalic.ttf", 32);
}

void GachaState::InitState() {
    BeginGachaOverlay(gStateAnim, 10, 0.6f, 1.2f, 0.3f);
}

void GachaState::Update(double dt) {
    bool skipPressed = AEInputCheckTriggered(AEVK_SPACE);
    bool exitPressed = AEInputCheckTriggered(AEVK_F);

    // 1. ESC to Quit Game (immediately breaks the loop in WinMain)
    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        return;
    }

    UpdateGachaOverlay(gStateAnim, (float)dt, skipPressed);

    // 2. Transition back to GameState only when finished and pressing F
    if (gStateAnim.phase == GachaPhase::Done && exitPressed) {
        // Matches your WinMain signature: Name, Restart flag, Init flag
        GameStateManager::GetInstance()->SetNextGameState("GameState", true, true);
    }
}

void GachaState::Draw() {
    // 1. Force a clear of the previous frame's settings
    AEGfxSetRenderMode(AE_GFX_RM_NONE);

    // 2. Set background manually here just in case
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // 3. Now call your gacha drawing logic
    // This function internally sets RM_COLOR and uses AEGfxPrint
    DrawGachaOverlay(gStateAnim, gachaFont);
}

void GachaState::ExitState() {
    gStateAnim.Reset();
}

void GachaState::UnloadState() {
    gachaFont = -1;
}