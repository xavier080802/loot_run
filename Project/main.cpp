#include <crtdbg.h> // Optional: check for memory leaks
#include "AEEngine.h"
#include "main.h"

// --------------------
// Game State Struct
// --------------------
typedef struct GameState {
    void (*init)(void);
    void (*update)(void);
    void (*render)(void);
    void (*exit)(void);
} GameState;

static GameState currentState = { nullptr, nullptr, nullptr, nullptr };

// --------------------
// World map globals
// --------------------
static AEGfxVertexList* circleMesh = nullptr;
static AEGfxVertexList* borderMesh = nullptr;
static AEVec2 playerPos;
static AEVec2 camPos;
static float playerRadius = 15.0f;
static float playerSpeed = 300.0f;
static float mapWidth = 2000.0f;
static float mapHeight = 2000.0f;
static float halfMapWidth;
static float halfMapHeight;

// --------------------
// Function declarations
// --------------------
static void InitWorldMap(void);
static void UpdateWorldMap(void);
static void RenderWorldMap(void);
static void ExitWorldMap(void);

void SetNextGameState(GameState newState);
void Terminate(void);

// --------------------
// Utility Functions
// --------------------
static AEGfxVertexList* CreateCircleMesh(int segments) {
    AEGfxMeshStart();
    for (int i = 0; i < segments; ++i) {
        f32 angle1 = TWO_PI * i / segments;
        f32 angle2 = TWO_PI * (i + 1) / segments;
        f32 x1 = AECos(angle1), y1 = AESin(angle1);
        f32 x2 = AECos(angle2), y2 = AESin(angle2);

        AEGfxTriAdd(0, 0, 0xFFFFFFFF, 0, 0,
            x1, y1, 0xFFFFFFFF, 0, 0,
            x2, y2, 0xFFFFFFFF, 0, 0);
    }
    return AEGfxMeshEnd();
}

static void FreeMesh(AEGfxVertexList** mesh) {
    if (*mesh) {
        AEGfxMeshFree(*mesh);
        *mesh = nullptr;
    }
}

// --------------------
// Entry Point (Windows subsystem)
// --------------------
int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int gGameRunning = 1;

    // Initialize AESys (window + engine)
    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);
    AESysSetWindowTitle("Camera Follow Test");
    AESysReset();

    // Enter first game state
    GameState worldMapState = { InitWorldMap, UpdateWorldMap, RenderWorldMap, ExitWorldMap };
    SetNextGameState(worldMapState);

    // --------------------
    // Game loop
    // --------------------
    while (gGameRunning)
    {
        AESysFrameStart();

        if (AEInputCheckTriggered(AEVK_ESCAPE) || !AESysDoesWindowExist())
            gGameRunning = 0;

        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

        if (currentState.update) currentState.update();
        if (currentState.render) currentState.render();

        AESysFrameEnd();
    }

    Terminate();
    return 0;
}

// --------------------
// Game state management
// --------------------
void SetNextGameState(GameState newState)
{
    if (currentState.exit) currentState.exit();
    currentState = newState;
    if (currentState.init) currentState.init();
}

void Terminate(void)
{
    if (currentState.exit) currentState.exit();
    AESysExit();
}

// --------------------
// Game State: Init
// --------------------
static void InitWorldMap(void)
{
    AEInputInit();

    halfMapWidth = mapWidth / 2.0f;
    halfMapHeight = mapHeight / 2.0f;

    circleMesh = CreateCircleMesh(36);

    AEGfxMeshStart();
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    borderMesh = AEGfxMeshEnd();

    AEVec2Zero(&playerPos);
    AEVec2Zero(&camPos);
}

// --------------------
// Game State: Update
// --------------------
static void UpdateWorldMap(void)
{
    f32 dt = AEFrameRateControllerGetFrameTime();
    AEInputUpdate();

    AEVec2 movement;
    AEVec2Zero(&movement);

    if (AEInputCheckCurr(AEVK_W)) movement.y += 1.0f;
    if (AEInputCheckCurr(AEVK_S)) movement.y -= 1.0f;
    if (AEInputCheckCurr(AEVK_A)) movement.x -= 1.0f;
    if (AEInputCheckCurr(AEVK_D)) movement.x += 1.0f;

    f32 len = AEVec2Length(&movement);
    if (len > 0.0f) {
        AEVec2Scale(&movement, &movement, playerSpeed * dt / len);
        AEVec2Add(&playerPos, &playerPos, &movement);
    }

    playerPos.x = AEClamp(playerPos.x, -halfMapWidth + playerRadius, halfMapWidth - playerRadius);
    playerPos.y = AEClamp(playerPos.y, -halfMapHeight + playerRadius, halfMapHeight - playerRadius);

    AEVec2Lerp(&camPos, &camPos, &playerPos, 0.1f);
}

// --------------------
// Game State: Render
// --------------------
static void RenderWorldMap(void)
{
    AEGfxStart();
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // Camera transform
    AEMtx33 camTrans, camScale, camFinal;
    f32 camZoom = 0.5f;
    AEMtx33Scale(&camScale, camZoom, camZoom);
    AEMtx33Trans(&camTrans, -camPos.x, -camPos.y);
    AEMtx33Concat(&camFinal, &camTrans, &camScale);

    // Border
    AEMtx33 borderScale, borderTrans, borderFinal, borderWorld;
    AEMtx33Scale(&borderScale, mapWidth, mapHeight);
    AEMtx33Trans(&borderTrans, 0.0f, 0.0f);
    AEMtx33Concat(&borderFinal, &borderTrans, &borderScale);
    AEMtx33Concat(&borderWorld, &camFinal, &borderFinal);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransform(borderWorld.m);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

    // Player
    AEMtx33 playerScale, playerTrans, playerFinal, playerWorld;
    AEMtx33Scale(&playerScale, playerRadius, playerRadius);
    AEMtx33Trans(&playerTrans, playerPos.x, playerPos.y);
    AEMtx33Concat(&playerFinal, &playerTrans, &playerScale);
    AEMtx33Concat(&playerWorld, &camFinal, &playerFinal);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransform(playerWorld.m);
    AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

    AEGfxEnd();
}

// --------------------
// Game State: Exit
// --------------------
static void ExitWorldMap(void)
{
    FreeMesh(&circleMesh);
    FreeMesh(&borderMesh);
}