//#include <crtdbg.h>
#include <math.h>             
#include "AEEngine.h"
#include "main.h"
#include "GameStates/GameStateManager.h"
#include "./GameStates/MainMenuState.h"
#include "./GameStates/LevelSelectState.h"
#include "./GameStates/GameState.h"
#include "./GameStates/ShopState.h"
#include "./GameStates/PetState.h"
#include "./GameObjects/GameObjectManager.h"
#include "UI/WorldText.h"
#include "./Helpers/RenderUtils.h"
#include "Rendering/RenderingManager.h"
#include "./Pets/PetManager.h"
#include "Elements/Element.h"
#include "UI/UIManager.h"
#include "InputManager.h"
#include "DebugTools.h"
#include "Drops/DropSystem.h"
#include "../Project/GameStates/CreditState.h"
#include "../Project/GameStates/LogoState.h"
#include "../Project/GameStates/GuideState.h"
#include "GameDB.h"
#if defined(DEBUG) | defined(_DEBUG)
	#include <memory>
#endif

namespace {
	InputManager* inputManager;
	UIManager* uiManager;
	GameStateManager* stateManager;
	GameObjectManager* goManager;
	RenderingManager* renderManager;
	PetManager* petManager;
	WorldTextManager* worldTextManager;

	bool gameRunningFlag{false};
}

//Entry point
int APIENTRY WinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	AESysInit(hInstance, nShowCmd, 1600, 900, 0, 60, false, NULL);

    AESysSetWindowTitle("Loot Run");
    AESysReset();
	gameRunningFlag = true;

    //Pre-loop setup

	inputManager = InputManager::GetInstance();
	uiManager = UIManager::GetInstance();
	goManager = GameObjectManager::GetInstance();
	renderManager = RenderingManager::GetInstance();
	petManager = PetManager::GetInstance();
	worldTextManager = WorldTextManager::GetInstance();
	inputManager->Init();
	uiManager->Init();
	renderManager->Init();
	petManager->Init();
	worldTextManager->Init();
	Elements::InitElementalSystem();
	DropSystem::InitDropSystemSettings();

	stateManager = GameStateManager::GetInstance();
	stateManager->AddGameState("MainMenuState", new MainMenuState);
	stateManager->AddGameState("LevelSelectState", new LevelSelectState);
	stateManager->AddGameState("GameState", new GameState);
	stateManager->AddGameState("ShopState", new ShopState);
	stateManager->AddGameState("PetState", new PetState);
	stateManager->AddGameState("CreditState", new CreditState);
	stateManager->AddGameState("LogoState", new LogoState);
	stateManager->AddGameState("GuideState", new GuideState);

	//Enter first game state
	stateManager->SetNextGameState("LogoState", true, true);

    while (AESysDoesWindowExist() && gameRunningFlag) {
        AESysFrameStart();

        // Let the Update handle the ESC key (or logic)
        f64 dt = AEFrameRateControllerGetFrameTime();

		uiManager->Update(); //Hover first to get overriden by click/release (if any)
		inputManager->Update();

		stateManager->UpdateCurrState(dt);

		worldTextManager->Update(dt);

        // ------------- Game loop logic -------------

		//In event of State terminating engine in Update, exit loop.
		if (!gameRunningFlag) break;

		//Rendering
		stateManager->DrawCurrState();
		worldTextManager->Draw();

		Debug::PrintLogs();

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
		GameDB::UnloadEquipmentReg();
		inputManager->Destroy();
		uiManager->Destroy();
		PostOffice::GetInstance()->Destroy();
		// free the system
		AESysExit();
	}
}