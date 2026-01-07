//#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "main.h"

static void (*stateInitFunc)(void);
static void (*stateUpdateFunc)(void);
static void (*stateExitFunc)(void);

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


	//Enter first game state
	//SetNextGameState(InitMainMenuState, UpdateMainMenuState, ExitMainMenuState);

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
		if (stateUpdateFunc) {
			stateUpdateFunc();
		}

		//------------- Informing the system about the loop's end -------------
		AESysFrameEnd();
	}

	Terminate();
}

void SetNextGameState(void(*init)(void), void(*update)(void), void(*exit)(void))
{
	//Exit previous state
	if (stateExitFunc) stateExitFunc();
	//Enter new state
	stateInitFunc = init;
	if (stateInitFunc) stateInitFunc();
	//Assign functions
	stateUpdateFunc = update;
	stateExitFunc = exit;
}

void Terminate(void)
{
	if (stateExitFunc) {
		stateExitFunc();
	}
	// free the system
	AESysExit();
}
