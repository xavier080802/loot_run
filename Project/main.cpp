//#include <crtdbg.h>
#include <math.h>             
#include "AEEngine.h"
#include "main.h"
#include "GameStateManager.h"
#include "./GameStates/MainMenuState.h"
#include "./GameStates/GameState.h"
#include "./GameObjects/GameObjectManager.h"
#include "./Helpers/RenderUtils.h"
#include "RenderingManager.h"

namespace {
	GameStateManager* stateManager;
	GameObjectManager* goManager;
	RenderingManager* renderManager;
}

//Entry point
int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);
    AESysSetWindowTitle("Loot Run");
    AESysReset();

    //Pre-loop setup
	goManager = GameObjectManager::GetInstance();
	renderManager = RenderingManager::GetInstance();
	renderManager->Init();

	stateManager = GameStateManager::GetInstance();
	stateManager->AddGameState("MainMenuState", new MainMenuState);
	stateManager->AddGameState("GameState", new GameState);

	//Enter first game state
	stateManager->SetNextGameState("GameState", true, true);

    while (AESysDoesWindowExist()) {
        AESysFrameStart();

        if (AEInputCheckTriggered(AEVK_ESCAPE))
            break;

        //------------- Game loop logic -------------
		//Clear bg
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
		
		//Update state logic
		f64 dt = AEFrameRateControllerGetFrameTime();
		stateManager->UpdateCurrState(dt);

		//Rendering
		stateManager->DrawCurrState();
        
		//------------- Informing the system about the loop's end -------------
        AESysFrameEnd();
    }

    Terminate();
    return 0;
}

void Terminate(void)
{
	stateManager->Destroy();
	goManager->Destroy();
	renderManager->Destroy();
	// free the system
	AESysExit();
}