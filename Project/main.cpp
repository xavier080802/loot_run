//#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "main.h"
#include "game_state_manager.h"
#include "./GameStates/main_menu_state.h"
#include "./GameStates/game_state.h"

static GameStateManager* gsm;

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
	gsm = GameStateManager::GetInstance();
	gsm->AddGameState("MainMenuState", new MainMenuState);
	gsm->AddGameState("GameState", new GameState);

	//Enter first game state
	gsm->SetNextGameState("MainMenuState");

	// Game Loop
	while (gGameRunning)
	{
		// Informing the system about the loop's start
		AESysFrameStart();

		// Basic way to trigger exiting the application
		// when ESCAPE is hit or when the window is closed
		if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
			gGameRunning = 0;

		//------------- Update logic -------------
		//Clear bg
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
		//Update state
		gsm->UpdateCurrState();

		//------------- Informing the system about the loop's end -------------
		AESysFrameEnd();
	}

	Terminate();
}


void Terminate(void)
{
	gsm->Destroy();
	// free the system
	AESysExit();
}
