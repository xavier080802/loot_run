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
    constexpr float POP_W  = 500.f;
    constexpr float POP_H  = 550.f;

    constexpr float POP_TOP = POP_CY - POP_H / 2.f;

    // Row Y positions inside the popup
    constexpr float ROW_TITLE    = POP_TOP + 50.f;
    constexpr float ROW_RESUME   = POP_TOP + 165.f;
    constexpr float ROW_SETTINGS = POP_TOP + 265.f;
    constexpr float ROW_GUIDE = POP_TOP + 365.f;
    constexpr float ROW_MAINMENU = POP_TOP + 465.f;

    // Button dimensions -- slightly wider than Settings close button (235)
    constexpr float BTN_W = 300.f;
    constexpr float BTN_H = 67.f;

    // Runtime state
    bool pauseOpen = false;

    // Hover tracking: [0]=resume  [1]=settings  [2]=mainmenu [3]=guide
    bool hoverStates[4] = { false };

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
        AEVec2  pos  = D2W(cx, cy, scale);
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
        AEVec2 pos  = D2W(cx, cy, scale);
        AEVec2 size = { w * scale, h * scale };
        return IsCursorOverWorld(pos, size.x, size.y, true);
    }

} // anonymous namespace

namespace Pause
{
    void Open()   { pauseOpen = true;  }
    void Close()  { pauseOpen = false; Settings::Close(); }
    void Toggle() { if (pauseOpen) Close(); else Open(); }
    bool IsOpen() { return pauseOpen; }

    bool Update()
    {
        if (!pauseOpen) return false;

        // Settings popup takes priority when open
        if (Settings::IsOpen())
        {
            // ESC or M while settings open closes settings, not pause
            Settings::Update(GetScale(), bgm.uiGroup,
                             bgm.uiClickSound, bgm.uiClickSound);
            return true;
        }

        bool clicked = AEInputCheckTriggered(AEVK_LBUTTON);
        float scale  = GetScale();

        // Resume -- [0]
        bool hovResume   = Hovered(POP_CX, ROW_RESUME,   BTN_W, BTN_H, scale);
        bool hovSettings = Hovered(POP_CX, ROW_SETTINGS, BTN_W, BTN_H, scale);
        bool hovMainMenu = Hovered(POP_CX, ROW_MAINMENU, BTN_W, BTN_H, scale);
        bool hovGuide = Hovered(POP_CX, ROW_GUIDE, BTN_W, BTN_H, scale);

        if (hovResume   && !hoverStates[0]) bgm.PlayUIClick();
        if (hovSettings && !hoverStates[1]) bgm.PlayUIClick();
        if (hovMainMenu && !hoverStates[2]) bgm.PlayUIClick();
        if (hovGuide && !hoverStates[3]) bgm.PlayUIClick();

        hoverStates[0] = hovResume;
        hoverStates[1] = hovSettings;
        hoverStates[2] = hovMainMenu;
        hoverStates[3] = hovGuide;

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
            else if (hovGuide) {
                bgm.PlayUIClick();
                GameStateManager::GetInstance()->SetNextGameState("GuideState", true, false, false);
            }
        }

        // ESC or M closes pause
        if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_M))
            Close();

        return true; // consumed this frame
    }

    void Draw()
    {
        if (!pauseOpen) return;

        float scale = GetScale();
        s8    font  = RenderingManager::GetInstance()->GetFont();

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

        // Guide button
        float gTint = hoverStates[3] ? 0.9f : 0.75f;
        DrawRect(POP_CX, ROW_GUIDE, BTN_W, BTN_H, scale, gTint, gTint, gTint);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        DrawAEText(font, "Guide",
                   D2W(POP_CX, ROW_GUIDE, scale), scale * 0.7f,
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

        // Draw Settings popup on top if it is open
        Settings::Draw(font, font, scale);
    }

} // namespace Pause
