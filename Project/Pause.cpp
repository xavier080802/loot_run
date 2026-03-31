#include "Pause.h"
#include "Settings.h"
#include "Music.h"
#include "Helpers/Vec2Utils.h"
#include "Helpers/CollisionUtils.h"
#include "Helpers/MatrixUtils.h"
#include "Helpers/RenderUtils.h"
#include "Helpers/ColorUtils.h"
#include "RenderingManager.h"
#include "GameStateManager.h"
#include "main.h"

namespace
{
    constexpr float DEFAULT_W = 1600.f;
    constexpr float DEFAULT_H = 900.f;

    // Popup dimensions (default-space)
    constexpr float POP_CX = DEFAULT_W / 2.f;
    constexpr float POP_CY = DEFAULT_H / 2.f;
    constexpr float POP_W = 500.f;
    constexpr float POP_H = 450.f;

    constexpr float POP_TOP = POP_CY - POP_H / 2.f;

    // Row Y positions inside the popup
    constexpr float ROW_TITLE = POP_TOP + 50.f;
    constexpr float ROW_RESUME = POP_TOP + 165.f;
    constexpr float ROW_SETTINGS = POP_TOP + 265.f;
    constexpr float ROW_MAINMENU = POP_TOP + 365.f;
    constexpr float ROW_HINT = POP_TOP + 600.f;

    // Button dimensions -- slightly wider than Settings close button (235)
    constexpr float BTN_W = 300.f;
    constexpr float BTN_H = 67.f;

    // Runtime state
    bool pauseOpen = false;

    // Hints
    const char* hintTexts[] = {
        "You can use the coins you earn to buy upgrades in the shop!",
        "Collect heath pickups to survive longer!",
        "It is ok to retreat from strong enemies!",
        "Keep an eye on your ammo and use your ranged attacks wisely!",
        "Your dodge has invincibility frames! Use it to avoid damage and reposition yourself!",
        "Try attacking and dodging at the same time!",
        "Pets can be helpful companions in battle. Press R to use your pet's ability!",
        "You can press and hold Left Click to auto attack!",
        "Try kiting enemies using ranged weapons!"
        //"Skill issue. Get Gud. L + Ratio"
    };
    constexpr int HINT_COUNT = sizeof(hintTexts) / sizeof(hintTexts[0]);
    int hintIndex = 0;
    bool hintsSeeded = false;

    // Hint rotation timer (seconds)
    constexpr float HINT_INTERVAL = 8.f;
    float hintTimer = 0.f;

    // Hover tracking: [0]=resume  [1]=settings  [2]=mainmenu
    bool hoverStates[3] = { false };

    float GetScale()
    {
        float winW = static_cast<float>(AEGfxGetWinMaxX()) * 2.f;
        float winH = static_cast<float>(AEGfxGetWinMaxY()) * 2.f;
        return (winW / DEFAULT_W) < (winH / DEFAULT_H)
            ? (winW / DEFAULT_W) : (winH / DEFAULT_H);
    }

    // Convert default-space (x, y) to HUD world coords
    AEVec2 D2W(float x, float y, float scale)
    {
        return {
            (x - DEFAULT_W / 2.f) * scale,
            (DEFAULT_H / 2.f - y) * scale
        };
    }

    AEGfxVertexList* GetMesh()
    {
        return RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
    }

    void DrawRect(float cx, float cy, float w, float h, float scale,
        float r, float g, float b, float a = 1.f)
    {
        AEVec2  pos = D2W(cx, cy, scale);
        AEVec2  size = { w * scale, h * scale };
        AEMtx33 mtx;
        GetTransformMtx(mtx, pos, 0.f, size);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);
        AEGfxSetTransform(mtx.m);
        AEGfxSetColorToMultiply(r, g, b, a);
        AEGfxMeshDraw(GetMesh(), AE_GFX_MDM_TRIANGLES);
    }

    bool Hovered(float cx, float cy, float w, float h, float scale)
    {
        AEVec2 pos = D2W(cx, cy, scale);
        AEVec2 size = { w * scale, h * scale };
        return IsCursorOverWorld(pos, size.x, size.y, true);
    }

} // anonymous namespace

namespace Pause
{
    void Open()
    {
        // Only pick a new hint when opening from closed state
        if (!pauseOpen)
        {
            if (!hintsSeeded)
            {
                std::srand(static_cast<unsigned>(std::time(nullptr)));
                hintsSeeded = true;
            }
            hintIndex = std::rand() % HINT_COUNT;
            hintTimer = 0.f;
        }
        pauseOpen = true;
    }
    void Close() { pauseOpen = false; Settings::Close(); }
    void Toggle() { if (pauseOpen) Close(); else Open(); }
    bool IsOpen() { return pauseOpen; }

    void Update(double& dt)
    {
        if (!pauseOpen) return;

        // Freeze the game world by killing the time step
        dt = 0.0;

        // Settings popup takes priority when open
        Settings::Update(GetScale());
        if (Settings::IsOpen()) return;

        bool clicked = AEInputCheckTriggered(AEVK_LBUTTON);
        float scale = GetScale();

        // Resume -- [0]
        bool hovResume = Hovered(POP_CX, ROW_RESUME, BTN_W, BTN_H, scale);
        bool hovSettings = Hovered(POP_CX, ROW_SETTINGS, BTN_W, BTN_H, scale);
        bool hovMainMenu = Hovered(POP_CX, ROW_MAINMENU, BTN_W, BTN_H, scale);

        if (hovResume && !hoverStates[0]) bgm.PlayUIHover();
        if (hovSettings && !hoverStates[1]) bgm.PlayUIHover();
        if (hovMainMenu && !hoverStates[2]) bgm.PlayUIHover();

        hoverStates[0] = hovResume;
        hoverStates[1] = hovSettings;
        hoverStates[2] = hovMainMenu;

        if (clicked)
        {
            if (hovResume)
            {
                bgm.PlayUIClick();
                Close();
            }
            else if (hovSettings)
            {
                bgm.PlayUIClick();
                Settings::Open();
            }
            else if (hovMainMenu)
            {
                bgm.PlayUIClick();
                Close();
                GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
            }
        }

        // ESC or M closes pause
        if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_M))
            Close();
    }

    void Draw()
    {
        if (!pauseOpen) return;

        float scale = GetScale();
        s8    font = RenderingManager::GetInstance()->GetFont();

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // Dim overlay
        DrawRect(DEFAULT_W / 2.f, DEFAULT_H / 2.f,
            DEFAULT_W, DEFAULT_H, scale, 0.f, 0.f, 0.f, 0.55f);

        // Panel background
        DrawRect(POP_CX, POP_CY, POP_W, POP_H, scale, 0.25f, 0.25f, 0.25f);

        // Title bar
        DrawRect(POP_CX, ROW_TITLE, POP_W, 100.f, scale, 0.75f, 0.75f, 0.75f);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        DrawAEText(font, "PAUSED",
            D2W(POP_CX, ROW_TITLE, scale), scale * 0.85f,
            CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        // Resume button
        float rTint = hoverStates[0] ? 0.9f : 0.75f;
        DrawRect(POP_CX, ROW_RESUME, BTN_W, BTN_H, scale, rTint, rTint, rTint);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        DrawAEText(font, "Resume",
            D2W(POP_CX, ROW_RESUME, scale), scale * 0.7f,
            CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        // Settings button
        float sTint = hoverStates[1] ? 0.9f : 0.75f;
        DrawRect(POP_CX, ROW_SETTINGS, BTN_W, BTN_H, scale, sTint, sTint, sTint);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        DrawAEText(font, "Settings",
            D2W(POP_CX, ROW_SETTINGS, scale), scale * 0.7f,
            CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        // Main Menu button
        float mTint = hoverStates[2] ? 0.9f : 0.75f;
        DrawRect(POP_CX, ROW_MAINMENU, BTN_W, BTN_H, scale, mTint, mTint, mTint);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        DrawAEText(font, "Main Menu",
            D2W(POP_CX, ROW_MAINMENU, scale), scale * 0.7f,
            CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        // Advance hint timer and rotate hints every interval
        hintTimer += static_cast<float>(AEFrameRateControllerGetFrameTime());
        if (hintTimer >= HINT_INTERVAL) {
            hintTimer = 0.f;
            hintIndex = (hintIndex + 1) % HINT_COUNT;
        }

        // Draw hint text
        const char* hintText = hintTexts[hintIndex % HINT_COUNT];
        char hintBuf[256];
        snprintf(hintBuf, sizeof(hintBuf), "Hint:  %s", hintText);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        DrawAEText(font, hintBuf,
            D2W(POP_CX, ROW_HINT, scale), scale * 0.6f,
            CreateColor(220, 220, 170, 255), TEXT_MIDDLE);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        // Draw Settings popup on top if it is open
        Settings::Draw(font, font, scale);
    }

} // namespace Pause