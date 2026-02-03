#include "main_menu_state.h"
#include "../Helpers/vec2_utils.h"
#include "../helpers/coord_utils.h"
#include <iostream>

static s8 menuFont = -1;

void MainMenuState::LoadState()
{
    menuFont = AEGfxCreateFont("Assets/Exo2/Exo2-SemiBoldItalic.ttf", 48);
}

void MainMenuState::InitState()
{
    std::cout << "Main menu state enter\n";
    AEGfxSetBackgroundColor(0.05f, 0.05f, 0.1f);
}

void MainMenuState::Update(double dt)
{
    UNREFERENCED_PARAMETER(dt);

    // 1. Press [1] to go to Game
    if (AEInputCheckTriggered(AEVK_1)) {
        GameStateManager::GetInstance()->SetNextGameState("GameState", true, true);
    }

    // 2. Press [2] to go to the Shop (Gacha)
    if (AEInputCheckTriggered(AEVK_2)) {
        GameStateManager::GetInstance()->SetNextGameState("GachaState", true, true);
    }

    // 3. Press [ESC] to Quit
    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        std::cout << "Closing application...\n";
        exit(0);
    }
}

void MainMenuState::Draw()
{
    AEGfxSetRenderMode(AE_GFX_RM_NONE);
    AEGfxPrint(menuFont, "LOOT RUN", -0.15f, 0.6f, 1.8f, 1.0f, 0.8f, 0.0f, 1.0f);
    AEGfxPrint(menuFont, "1 : START GAME", -0.2f, 0.1f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxPrint(menuFont, "2 : ENTER SHOP", -0.2f, -0.1f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxPrint(menuFont, "[ESC] TO QUIT", -0.15f, -0.8f, 0.7f, 0.5f, 0.5f, 0.5f, 1.0f);
    AEGfxSetRenderMode(AE_GFX_RM_NONE);
}

void MainMenuState::ExitState()
{
    std::cout << "Exit main menu state\n";
}

void MainMenuState::UnloadState()
{
    menuFont = -1;
}
