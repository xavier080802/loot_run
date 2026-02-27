//#include <crtdbg.h>
#include <math.h>             
#include "AEEngine.h"
#include "main.h"
#include "GameStateManager.h"
#include "./GameStates/MainMenuState.h"
#include "./GameStates/GameState.h"
#include "./GameStates/ShopState.h"
#include "./GameObjects/GameObjectManager.h"
#include "WorldText.h"
#include "./Helpers/RenderUtils.h"
#include "RenderingManager.h"
#include "./Pets/PetManager.h"
#include "Elements/Element.h"

namespace {
	GameStateManager* stateManager;
	GameObjectManager* goManager;
	RenderingManager* renderManager;
	PetManager* petManager;
	WorldTextManager* worldTextManager;

	bool gameRunningFlag{false};
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
	gameRunningFlag = true;

    //Pre-loop setup
	goManager = GameObjectManager::GetInstance();
	renderManager = RenderingManager::GetInstance();
	petManager = PetManager::GetInstance();
	worldTextManager = WorldTextManager::GetInstance();
	renderManager->Init();
	petManager->Init();
	worldTextManager->Init();
	Elements::InitElementalSystem();

	stateManager = GameStateManager::GetInstance();
	stateManager->AddGameState("MainMenuState", new MainMenuState);
	stateManager->AddGameState("GameState", new GameState);
	stateManager->AddGameState("ShopState", new ShopState);

	//Enter first game state
	stateManager->SetNextGameState("MainMenuState", true, true);

    while (AESysDoesWindowExist() && gameRunningFlag) {
        AESysFrameStart();

        // Let the Update handle the ESC key (or logic)
        f64 dt = AEFrameRateControllerGetFrameTime();

		stateManager->UpdateCurrState(dt);

		worldTextManager->Update(dt);

        // ------------- Game loop logic -------------

		//In event of State terminating engine in Update, exit loop.
		if (!gameRunningFlag) break;

		//Rendering
		stateManager->DrawCurrState();
		worldTextManager->Draw();
        
		//------------- Informing the system about the loop's end -------------
        AESysFrameEnd();
    }

    Terminate();
    return 0;
}

void Terminate(void)
{
	if (gameRunningFlag)
	{
		gameRunningFlag = false;
		stateManager->Destroy();
		petManager->Destroy();
		goManager->Destroy();
		renderManager->Destroy();
		worldTextManager->Destroy();
		// free the system
		AESysExit();
	}
}