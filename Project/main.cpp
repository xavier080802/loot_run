#include <crtdbg.h>
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
static float camZoom = 1.0f;   // default zoom

// --------------------
// Minimap
// --------------------
static float minimapWidth = 200.0f;
static float minimapHeight = 200.0f;
static float minimapMargin = 20.0f;

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

// --------------------
// Entry Point
// --------------------
int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, NULL);
    AESysSetWindowTitle("Loot Run");
    AESysReset();

    GameState worldMapState = { InitWorldMap, UpdateWorldMap, RenderWorldMap, ExitWorldMap };
    SetNextGameState(worldMapState);

    while (AESysDoesWindowExist())
    {
        AESysFrameStart();

        if (AEInputCheckTriggered(AEVK_ESCAPE))
            break;

        AEGfxSetBackgroundColor(0.1f, 0.1f, 0.1f);

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

    // Map border
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

    if (AEInputCheckCurr(AEVK_W)) movement.y += 5.0f;
    if (AEInputCheckCurr(AEVK_S)) movement.y -= 5.0f;
    if (AEInputCheckCurr(AEVK_A)) movement.x -= 5.0f;
    if (AEInputCheckCurr(AEVK_D)) movement.x += 5.0f;

    f32 len = AEVec2Length(&movement);
    if (len > 0.0f) {
        AEVec2Scale(&movement, &movement, playerSpeed * dt / len);
        AEVec2Add(&playerPos, &playerPos, &movement);
    }

    playerPos.x = AEClamp(playerPos.x, -halfMapWidth + playerRadius, halfMapWidth - playerRadius);
    playerPos.y = AEClamp(playerPos.y, -halfMapHeight + playerRadius, halfMapHeight - playerRadius);

    // Smooth camera follow
    f32 camSpeed = 5.5f;
    f32 factor = camSpeed * dt;
    if (factor > 1.0f) factor = 1.0f;
    AEVec2 temp;
    AEVec2Lerp(&temp, &camPos, &playerPos, factor);
    camPos = temp;

    // Zoom controls
    //if (AEInputCheckCurr(AEVK_Q)) camZoom += 1.0f * dt; // zoom in
    //if (AEInputCheckCurr(AEVK_E)) camZoom -= 1.0f * dt; // zoom out
    camZoom = AEClamp(camZoom, 0.5f, 2.5f);
}





















// --------------------
// Game State: Render
// --------------------
static void RenderWorldMap(void)
{
    AEGfxStart();
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // Camera transform
    AEMtx33 camTrans, camScale, camFinal;
    AEMtx33Scale(&camScale, camZoom, camZoom);
    AEMtx33Trans(&camTrans, -camPos.x, -camPos.y);
    AEMtx33Concat(&camFinal, &camTrans, &camScale);

    // --- Map border ---
    AEMtx33 borderScale, borderTrans, borderFinal, borderWorld;
    AEMtx33Scale(&borderScale, mapWidth, mapHeight);
    AEMtx33Trans(&borderTrans, 0.0f, 0.0f);
    AEMtx33Concat(&borderFinal, &borderTrans, &borderScale);
    AEMtx33Concat(&borderWorld, &camFinal, &borderFinal);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransform(borderWorld.m);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

    // --- Player ---
    AEMtx33 playerScale, playerTrans, playerFinal, playerWorld;
    AEMtx33Scale(&playerScale, playerRadius, playerRadius);
    AEMtx33Trans(&playerTrans, playerPos.x, playerPos.y);
    AEMtx33Concat(&playerFinal, &playerTrans, &playerScale);
    AEMtx33Concat(&playerWorld, &camFinal, &playerFinal);
    AEGfxSetTransform(playerWorld.m);
    AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);
    
    // --- Minimap rendering --- 
    float winW = (float)AEGfxGetWinMaxX();
    float winH = (float)AEGfxGetWinMaxY();

    // scale factors (ensure float division)
    float scaleX = (float)minimapWidth / (float)mapWidth;
    float scaleY = (float)minimapHeight / (float)mapHeight;

    // anchor minimap to top-right corner
    float screenRight = winW - minimapWidth * 0.5f - minimapMargin;
    float screenTop = winH - minimapHeight * 0.5f - minimapMargin;

    // --- Minimap Border ---
    AEMtx33 mmBorderScale, mmBorderTrans, mmBorderFinal;
    AEMtx33Scale(&mmBorderScale, mapWidth * scaleX, mapHeight * scaleY);
    AEMtx33Trans(&mmBorderTrans, screenRight, screenTop);
    AEMtx33Concat(&mmBorderFinal, &mmBorderTrans, &mmBorderScale);
    AEGfxSetTransform(mmBorderFinal.m);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

    // --- Player in Minimap ---
    AEMtx33 mmPlayerScale, mmPlayerTrans, mmPlayerFinal;
    AEMtx33Scale(&mmPlayerScale, playerRadius * scaleX, playerRadius * scaleY);
    AEMtx33Trans(&mmPlayerTrans,
        screenRight + playerPos.x * scaleX,
        screenTop + playerPos.y * scaleY);
    AEMtx33Concat(&mmPlayerFinal, &mmPlayerTrans, &mmPlayerScale);
    AEGfxSetTransform(mmPlayerFinal.m);
    AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

    // --- Camera viewport ---
    float viewW = winW / camZoom;
    float viewH = winH / camZoom;
    AEMtx33 mmViewScale, mmViewTrans, mmViewFinal;
    AEMtx33Scale(&mmViewScale, viewW * scaleX, viewH * scaleY);
    AEMtx33Trans(&mmViewTrans,
        screenRight + camPos.x * scaleX,
        screenTop + camPos.y * scaleY);
    AEMtx33Concat(&mmViewFinal, &mmViewTrans, &mmViewScale);
    AEGfxSetTransform(mmViewFinal.m);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

}

// --------------------
// Game State: Exit
// --------------------
static void ExitWorldMap(void)
{
    if (circleMesh) { AEGfxMeshFree(circleMesh); circleMesh = nullptr; }
    if (borderMesh) { AEGfxMeshFree(borderMesh); borderMesh = nullptr; }
}
