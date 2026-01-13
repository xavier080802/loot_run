//#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "main.h"
#include "game_state_manager.h"
#include "./GameStates/main_menu_state.h"
#include "./GameStates/game_state.h"
#include "./GameObjects/GameObjectManager.h"
#include "rendering_manager.h"

static GameStateManager* stateManager;
static GameObjectManager* goManager;
static RenderingManager* renderManager;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	int gGameRunning = 1;

	// Initialization of your own variables go here

	// Using custom window procedure
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);

	// Changing the window title
	AESysSetWindowTitle("Project");

	// reset the system modules
	AESysReset();

	//Pre-loop setup
	goManager = GameObjectManager::GetInstance();
	renderManager = RenderingManager::GetInstance();
	renderManager->Init();

	stateManager = GameStateManager::GetInstance();
	stateManager->AddGameState("MainMenuState", new MainMenuState);
	stateManager->AddGameState("GameState", new GameState);

	//Enter first game state
	stateManager->SetNextGameState("MainMenuState");

	// Game Loop
	while (gGameRunning)
	{
		// Informing the system about the loop's start
		AESysFrameStart();

		// Basic way to trigger exiting the application
		// when ESCAPE is hit or when the window is closed
		if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
			gGameRunning = 0;

		//------------- Game loop logic -------------
		//Clear bg
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
		
		//Update state
		f64 dt = AEFrameRateControllerGetFrameTime();
		stateManager->UpdateCurrState(dt);
		goManager->UpdateObjects(dt);

		//Rendering
		goManager->DrawObjects();

		//------------- Informing the system about the loop's end -------------
		AESysFrameEnd();
	}

	Terminate();
}


void Terminate(void)
{
	stateManager->Destroy();
	goManager->Destroy();
	renderManager->Destroy();
	// free the system
	AESysExit();
}
