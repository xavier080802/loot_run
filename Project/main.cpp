
#include <crtdbg.h>
#include <math.h>              // for sqrtf
#include "AEEngine.h"
#include "main.h"

// --------------------
// Game State
// --------------------
typedef struct GameState {
    void (*init)(void);
    void (*update)(void);
    void (*render)(void);
    void (*exit)(void);
} GameState;

static GameState currentState = { nullptr, nullptr, nullptr, nullptr };

// --------------------
// Globals
// --------------------
static AEGfxVertexList* circleMesh = nullptr;
static AEGfxVertexList* borderMesh = nullptr;   // single border mesh (white)

static AEVec2 playerPos;
static AEVec2 playerDir = { 1.0f, 0.0f };      // last non-zero movement direction
static AEVec2 camPos;

static float playerRadius = 15.0f;
static float playerSpeed = 300.0f;

static float mapWidth = 2000.0f;
static float mapHeight = 2000.0f;
static float halfMapWidth;
static float halfMapHeight;

static float camZoom = 1.0f;   // default zoom

// Minimap
static float minimapWidth = 200.0f;
static float minimapHeight = 200.0f;
static float minimapMargin = 20.0f;

// --- Camera smoothing state/params ---
static AEVec2 camVel = { 0.0f, 0.0f };     // SmoothDamp velocity accumulator
static float  camSmoothTime = 0.25f;       // lower = snappier, higher = smoother (0.15f–0.35f good)

// --- Soft edge easing (distance from edges where easing begins) ---
static float softMargin = 100.0f;          // world units (tune 60–200)

// --- Minimap tuning ---
static float minimapPlayerScaleFactor = 1.25f;  // <1.0 to shrink the player dot on minimap

// --- Minimap Arrow (size + color) ---
// Smaller triangle: tune these to your liking
static float minimapArrowLengthPx = 10.0f;        // tip-to-tail length in pixels (was 18)
static float minimapArrowThicknessPx = 6.0f;         // base thickness in pixels (was 12)
// Extra spacing beyond player dot radius, keeps the arrow separated
static float minimapArrowExtraOffsetPx = 6.0f;       // extra margin beyond circle radius
static unsigned int minimapArrowColor = 0xFF00FFFF;

// --------------------
// Forward declarations
// --------------------
static void InitWorldMap(void);
static void UpdateWorldMap(void);
static void RenderWorldMap(void);
static void ExitWorldMap(void);

void SetNextGameState(GameState newState);
void Terminate(void);

// --------------------
// Small helpers (no lambdas)
// --------------------
static inline float Saturate(float x) { return AEClamp(x, 0.0f, 1.0f); }
static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

// --------------------
// Mesh builders
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

static AEGfxVertexList* CreateBorderRectMesh(void) {
    AEGfxMeshStart();
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
    AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
    return AEGfxMeshEnd();
}

// --------------------
// WinMain
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
    borderMesh = CreateBorderRectMesh();

    AEVec2Zero(&playerPos);
    playerDir.x = 1.0f; playerDir.y = 0.0f;

    AEVec2Zero(&camPos);
    AEVec2Zero(&camVel);
}

// --------------------
// Game State: Update
// --------------------
static void UpdateWorldMap(void)
{
    f32 dt = AEFrameRateControllerGetFrameTime();
    AEInputUpdate();

    // --- Player movement ---
    AEVec2 movement;
    AEVec2Zero(&movement);

    if (AEInputCheckCurr(AEVK_W)) movement.y += 5.0f;
    if (AEInputCheckCurr(AEVK_S)) movement.y -= 5.0f;
    if (AEInputCheckCurr(AEVK_A)) movement.x -= 5.0f;
    if (AEInputCheckCurr(AEVK_D)) movement.x += 5.0f;

    f32 len = AEVec2Length(&movement);
    if (len > 0.0f) {
        // Normalize and cache facing direction
        AEVec2 dirN = movement;
        AEVec2Scale(&dirN, &dirN, 1.0f / len);
        playerDir = dirN;

        // Move with speed
        AEVec2Scale(&movement, &movement, playerSpeed * dt / len);
        AEVec2Add(&playerPos, &playerPos, &movement);
    }

    // Clamp player inside map
    playerPos.x = AEClamp(playerPos.x, -halfMapWidth + playerRadius, halfMapWidth - playerRadius);
    playerPos.y = AEClamp(playerPos.y, -halfMapHeight + playerRadius, halfMapHeight - playerRadius);

    // --------------------
    // Smooth camera follow with SOFT EASING near edges
    // --------------------
    float winW = (float)AEGfxGetWinMaxX();
    float winH = (float)AEGfxGetWinMaxY();

    // Half viewport size in world units (account for zoom)
    float halfViewW = (winW * 0.5f) / camZoom;
    float halfViewH = (winH * 0.5f) / camZoom;

    // Safe bounds for camera center (exact)
    float minX = -halfMapWidth + halfViewW;
    float maxX = halfMapWidth - halfViewW;
    float minY = -halfMapHeight + halfViewH;
    float maxY = halfMapHeight - halfViewH;

    // Handle case where viewport is larger than map on an axis
    if (minX > maxX) { minX = maxX = 0.0f; }
    if (minY > maxY) { minY = maxY = 0.0f; }

    AEVec2 camTarget = playerPos;

    // X axis soft easing
    float tLeft = 1.0f - Saturate((playerPos.x - (minX + softMargin)) / softMargin);
    float tRight = 1.0f - Saturate(((maxX - softMargin) - playerPos.x) / softMargin);
    camTarget.x = Lerp(playerPos.x, minX, tLeft);
    camTarget.x = Lerp(camTarget.x, maxX, tRight);

    // Y axis soft easing
    float tBottom = 1.0f - Saturate((playerPos.y - (minY + softMargin)) / softMargin);
    float tTop = 1.0f - Saturate(((maxY - softMargin) - playerPos.y) / softMargin);
    camTarget.y = Lerp(playerPos.y, minY, tBottom);
    camTarget.y = Lerp(camTarget.y, maxY, tTop);

    // SmoothDamp (Unity-like), stable for variable dt
    float tSmooth = camSmoothTime;
    if (tSmooth < 0.0001f) tSmooth = 0.0001f;

    float omega = 2.0f / tSmooth;
    float xd = omega * dt;
    float expK = 1.0f / (1.0f + xd + 0.48f * xd * xd + 0.235f * xd * xd * xd);

    // change = current - target
    AEVec2 change = { camPos.x - camTarget.x, camPos.y - camTarget.y };
    // temp = (camVel + omega * change) * dt
    AEVec2 temp = { (camVel.x + omega * change.x) * dt,
                      (camVel.y + omega * change.y) * dt };

    // camVel = (camVel - omega * temp) * expK
    camVel.x = (camVel.x - omega * temp.x) * expK;
    camVel.y = (camVel.y - omega * temp.y) * expK;

    // camPos = camTarget + (change + temp) * expK
    camPos.x = camTarget.x + (change.x + temp.x) * expK;
    camPos.y = camTarget.y + (change.y + temp.y) * expK;

    // Final hard clamp (safety, avoids rare overshoot)
    camPos.x = AEClamp(camPos.x, minX, maxX);
    camPos.y = AEClamp(camPos.y, minY, maxY);

    // --- Zoom controls (optional) ---
    // if (AEInputCheckCurr(AEVK_Q)) camZoom += 1.0f * dt; // zoom in
    // if (AEInputCheckCurr(AEVK_E)) camZoom -= 1.0f * dt; // zoom out
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

    // --- Map border (world) ---
    AEMtx33 borderScale, borderTrans, borderFinal, borderWorld;
    AEMtx33Scale(&borderScale, mapWidth, mapHeight);
    AEMtx33Trans(&borderTrans, 0.0f, 0.0f);
    AEMtx33Concat(&borderFinal, &borderTrans, &borderScale);
    AEMtx33Concat(&borderWorld, &camFinal, &borderFinal);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransform(borderWorld.m);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

    // --- Player (world) ---
    AEMtx33 playerScale, playerTrans, playerFinal, playerWorld;
    AEMtx33Scale(&playerScale, playerRadius, playerRadius);
    AEMtx33Trans(&playerTrans, playerPos.x, playerPos.y);
    AEMtx33Concat(&playerFinal, &playerTrans, &playerScale);
    AEMtx33Concat(&playerWorld, &camFinal, &playerFinal);
    AEGfxSetTransform(playerWorld.m);
    AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

    // --------------------
    // Minimap rendering
    // --------------------
    float winW = (float)AEGfxGetWinMaxX();
    float winH = (float)AEGfxGetWinMaxY();

    // scale factors (minimap <-> world)
    float scaleX = (float)minimapWidth / (float)mapWidth;
    float scaleY = (float)minimapHeight / (float)mapHeight;

    // anchor minimap to top-right corner
    float screenRight = winW - minimapWidth * 0.5f - minimapMargin;
    float screenTop = winH - minimapHeight * 0.5f - minimapMargin;

    // --- Minimap Border (map box, white) ---
    AEMtx33 mmBorderScale, mmBorderTrans, mmBorderFinal;
    AEMtx33Scale(&mmBorderScale, mapWidth * scaleX, mapHeight * scaleY);
    AEMtx33Trans(&mmBorderTrans, screenRight, screenTop);
    AEMtx33Concat(&mmBorderFinal, &mmBorderTrans, &mmBorderScale);
    AEGfxSetTransform(mmBorderFinal.m);
    AEGfxMeshDraw(borderMesh, AE_GFX_MDM_LINES_STRIP);

    // --- Player in Minimap (circle) ---
    AEMtx33 mmPlayerScale, mmPlayerTrans, mmPlayerFinal;
    AEMtx33Scale(&mmPlayerScale,
        (playerRadius * scaleX) * minimapPlayerScaleFactor,
        (playerRadius * scaleY) * minimapPlayerScaleFactor);
    AEMtx33Trans(&mmPlayerTrans,
        screenRight + playerPos.x * scaleX,
        screenTop + playerPos.y * scaleY);
    AEMtx33Concat(&mmPlayerFinal, &mmPlayerTrans, &mmPlayerScale);
    AEGfxSetTransform(mmPlayerFinal.m);
    AEGfxMeshDraw(circleMesh, AE_GFX_MDM_TRIANGLES);

    // --- Facing direction on MINIMAP (smaller triangle, custom color, separated from circle) ---
    {
        // Player dot radius in minimap pixels (accounts for anisotropic scaling)
        float rpx = (playerRadius * scaleX) * minimapPlayerScaleFactor;
        float rpy = (playerRadius * scaleY) * minimapPlayerScaleFactor;
        float rAvg = 0.5f * (rpx + rpy);

        // Separation: radius + extra margin
        float arrowOffsetPx = rAvg + minimapArrowExtraOffsetPx;

        // Direction basis in MINIMAP PIXEL SPACE (handles scaleX != scaleY)
        float dx = playerDir.x * scaleX;
        float dy = playerDir.y * scaleY;
        float mag = (float)sqrtf(dx * dx + dy * dy);
        if (mag < 1e-4f) {         // default to pointing right if stationary
            dx = 1.0f; dy = 0.0f;
            mag = 1.0f;
        }
        dx /= mag; dy /= mag;      // unit direction in minimap pixels

        // Perpendicular in minimap pixels
        float px = -dy;
        float py = dx;

        // Player position in minimap pixel coordinates
        float cx = screenRight + playerPos.x * scaleX;
        float cy = screenTop + playerPos.y * scaleY;

        // Arrow center offset outward from the circle
        float ax = cx + dx * arrowOffsetPx;
        float ay = cy + dy * arrowOffsetPx;

        float Lpx = minimapArrowLengthPx;
        float Tpx = minimapArrowThicknessPx;
        float baseBack = Lpx * 0.3f;

        float p0x = ax + dx * (Lpx * 0.7f);           // tip
        float p0y = ay + dy * (Lpx * 0.7f);

        float p1x = ax - dx * baseBack + px * (Tpx * 0.5f); // base left
        float p1y = ay - dy * baseBack + py * (Tpx * 0.5f);

        float p2x = ax - dx * baseBack - px * (Tpx * 0.5f); // base right
        float p2y = ay - dy * baseBack - py * (Tpx * 0.5f);

        // Draw the triangle directly in screen space (identity transform)
        AEGfxMeshStart();
        AEGfxTriAdd(p0x, p0y, minimapArrowColor, 0, 0,
            p1x, p1y, minimapArrowColor, 0, 0,
            p2x, p2y, minimapArrowColor, 0, 0);
        AEGfxVertexList* miniArrowOnce = AEGfxMeshEnd();

        AEMtx33 idS, idT, idM;
        AEMtx33Scale(&idS, 1.0f, 1.0f);
        AEMtx33Trans(&idT, 0.0f, 0.0f);
        AEMtx33Concat(&idM, &idT, &idS);
        AEGfxSetTransform(idM.m);
        AEGfxMeshDraw(miniArrowOnce, AE_GFX_MDM_TRIANGLES);
        if (miniArrowOnce) AEGfxMeshFree(miniArrowOnce);
    }

}

// --------------------
// Game State: Exit
// --------------------
static void ExitWorldMap(void)
{
    if (circleMesh) { AEGfxMeshFree(circleMesh); circleMesh = nullptr; }
    if (borderMesh) { AEGfxMeshFree(borderMesh); borderMesh = nullptr; }
}
