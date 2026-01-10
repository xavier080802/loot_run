#include "AEEngine.h"
#include "main.h"
#include "game_state.h"

static void (*stateUpdateFunc)(void);
static void (*stateDrawFunc)(void);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, nullptr);
    AESysSetWindowTitle("Alpha Engine Demo");
    AESysReset();

    GameState_Init();
    stateUpdateFunc = GameState_Update;
    stateDrawFunc = GameState_Draw;

    bool gGameRunning = true;
    while (gGameRunning)
    {
        AESysFrameStart();

        if (AEInputCheckTriggered(AEVK_ESCAPE) || !AESysDoesWindowExist())
            gGameRunning = false;

        AEGfxSetBackgroundColor(0.1f, 0.1f, 0.1f); // dark gray background

        if (stateUpdateFunc) stateUpdateFunc();
        if (stateDrawFunc) stateDrawFunc();

        AESysFrameEnd();
    }

    AESysExit();
    return 0;
}
