#include "Settings.h"
#include "Music.h"
#include "Helpers/Vec2Utils.h"
#include "Helpers/CollisionUtils.h"
#include "Helpers/MatrixUtils.h"
#include "Helpers/RenderUtils.h"
#include "Helpers/ColorUtils.h"
#include "RenderingManager.h"
#include <fstream>
#include <json/json.h>
#include <iostream>

// Internal state
namespace
{
    constexpr float DEFAULT_W = 1600.f;
    constexpr float DEFAULT_H = 900.f;

    // Popup dimensions (default-space)
    constexpr float POP_CX = DEFAULT_W / 2.f;
    constexpr float POP_CY = DEFAULT_H / 2.f;
    constexpr float POP_W  = 600.f;
    constexpr float POP_H  = 500.f;

    constexpr float POP_TOP = POP_CY - POP_H / 2.f;

    // Row Y positions inside the popup
    constexpr float ROW_TITLE = POP_TOP + 55.f;
    constexpr float ROW_BGM = POP_TOP + 140.f;
    constexpr float ROW_UI    = POP_TOP + 240.f;
    constexpr float ROW_SFX   = POP_TOP + 340.f;
    constexpr float ROW_CLOSE = POP_TOP + 450.f;

    // Volume slider dimensions
    constexpr float SLIDER_W    = 300.f;
    constexpr float SLIDER_H    = 60.f;
    constexpr float SIDE_W      = 60.f;
    constexpr float SIDE_H      = 60.f;
    constexpr float SIDE_OFFSET = (SLIDER_W / 2.f) + (SIDE_W / 2.f) + 4.f;

    // Runtime state
    bool popupOpen   = false;
    int  bgmVolume = 5;  // 0-10. Note: Initial value set in Music.cpp Init, match this value with that
    int  uiVolume    = 10;  // 0-10
    int  sfxVolume   = 10;  // 0-10

    static constexpr const char* SETTINGS_PATH = "Assets/Data/Player/player_data.json";

    void SaveSettings()
    {
        // Read existing file first so other nodes are preserved
        Json::Value root;
        {
            std::ifstream in(SETTINGS_PATH);
            if (in.is_open())
            {
                Json::CharReaderBuilder builder;
                std::string errs;
                Json::parseFromStream(builder, in, &root, &errs);
            }
        }

        Json::Value settingsNode(Json::objectValue);
        settingsNode["bgmVolume"] = bgmVolume;
        settingsNode["uiVolume"]  = uiVolume;
        settingsNode["sfxVolume"] = sfxVolume;
        root["settings"]  = settingsNode;

        std::ofstream out(SETTINGS_PATH);
        if (!out.is_open())
        {
            std::cout << "[Settings] Could not open " << SETTINGS_PATH << " for writing.\n";
            return;
        }
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "    ";
        out << Json::writeString(writer, root);
        std::cout << "[Settings] Saved volumes BGM=" << bgmVolume
                  << " UI=" << uiVolume << " SFX=" << sfxVolume << "\n";
    }

    void LoadSettings()
    {
        std::ifstream in(SETTINGS_PATH);
        if (!in.is_open())
        {
            std::cout << "[Settings] " << SETTINGS_PATH << " not found, using defaults.\n";
            return;
        }
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;
        if (!Json::parseFromStream(builder, in, &root, &errs))
        {
            std::cout << "[Settings] Parse error: " << errs << "\n";
            return;
        }
        const Json::Value& node = root["settings"];
        if (!node.isObject()) return;

        bgmVolume = node.get("bgmVolume", bgmVolume).asInt();
        uiVolume  = node.get("uiVolume",  uiVolume).asInt();
        sfxVolume = node.get("sfxVolume", sfxVolume).asInt();
        std::cout << "[Settings] Loaded volumes BGM=" << bgmVolume
                  << " UI=" << uiVolume << " SFX=" << sfxVolume << "\n";
    }

    // Hover tracking
    // [0]=close
    // [1]=music-minus  [2]=music-plus
    // [3]=ui-minus     [4]=ui-plus
    // [5]=sfx-minus    [6]=sfx-plus
    bool hoverStates[7] = { false };

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

    // Draw one volume row. Highlight state passed in from hoverStates.
    void DrawVolumeRow(s8 font, float rowY, float scale,
                       const char* label, int value,
                       bool hoverMinus, bool hoverPlus)
    {
        // Centre label block
        DrawRect(POP_CX, rowY, SLIDER_W, SLIDER_H, scale, 0.65f, 0.65f, 0.65f);
        char buf[64];
        snprintf(buf, sizeof(buf), "%s: %d%%", label, value * 10);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        DrawAEText(font, buf, D2W(POP_CX, rowY, scale), scale,
                   CreateColor(10, 10, 10, 255), TEXT_MIDDLE);

        // Minus button (red)
        DrawRect(POP_CX - SIDE_OFFSET, rowY, SIDE_W, SIDE_H, scale,
                 hoverMinus ? 0.9f : 0.75f,
                 hoverMinus ? 0.3f : 0.2f,
                 hoverMinus ? 0.3f : 0.2f);
        DrawAEText(font, "-",
                   D2W(POP_CX - SIDE_OFFSET, rowY+10, scale), scale,
                   CreateColor(255, 255, 255, 255), TEXT_MIDDLE);

        // Plus button (green)
        DrawRect(POP_CX + SIDE_OFFSET, rowY, SIDE_W, SIDE_H, scale,
                 hoverPlus ? 0.3f : 0.2f,
                 hoverPlus ? 0.9f : 0.75f,
                 hoverPlus ? 0.3f : 0.2f);
        DrawAEText(font, "+",
                   D2W(POP_CX + SIDE_OFFSET, rowY+5, scale), scale,
                   CreateColor(255, 255, 255, 255), TEXT_MIDDLE);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    }

    // Process one volume row's clicks. Updates value and calls the setter.
    // minusIdx / plusIdx: indices into hoverStates for this row.
    void UpdateVolumeRow(float rowY, float scale,
                         int& volume, void (BGMManager::*setter)(float),
                         int minusIdx, int plusIdx,
                         bool clicked)
    {
        bool minus = Hovered(POP_CX - SIDE_OFFSET, rowY, SIDE_W, SIDE_H, scale);
        bool plus  = Hovered(POP_CX + SIDE_OFFSET, rowY, SIDE_W, SIDE_H, scale);

        if (minus && !hoverStates[minusIdx]) bgm.PlayUIHover();
        if (plus  && !hoverStates[plusIdx])  bgm.PlayUIHover();
        hoverStates[minusIdx] = minus;
        hoverStates[plusIdx]  = plus;

        if (clicked)
        {
            if (minus && volume > 0)
            {
                --volume;
                (bgm.*setter)(volume / 10.f);
                bgm.PlayUIClick();
                SaveSettings();
            }
            else if (plus && volume < 10)
            {
                ++volume;
                (bgm.*setter)(volume / 10.f);
                bgm.PlayUIClick();
                SaveSettings();
            }
        }
    }

} // anonymous namespace

// Public interface
namespace Settings
{
    void Open()  { LoadSettings(); popupOpen = true; }
    void Close() { popupOpen = false; }
    bool IsOpen(){ return popupOpen;  }

    int GetBGMVolume() { return bgmVolume; }
    int GetUIVolume()    { return uiVolume;    }
    int GetSFXVolume()   { return sfxVolume;   }

    void Load()
    {
        LoadSettings();
        bgm.SetBGMVolume(bgmVolume / 10.f);
        bgm.SetUIVolume(uiVolume   / 10.f);
        bgm.SetSFXVolume(sfxVolume / 10.f);
    }

    bool Update(float scale)
    {
        if (!popupOpen) return false;

        bool clicked = AEInputCheckTriggered(AEVK_LBUTTON);

        // Music row  -- hoverStates[1], [2]
        UpdateVolumeRow(ROW_BGM, scale,
                        bgmVolume, &BGMManager::SetBGMVolume,
                        1, 2, clicked);

        // UI row  -- hoverStates[3], [4]
        UpdateVolumeRow(ROW_UI, scale,
                        uiVolume, &BGMManager::SetUIVolume,
                        3, 4, clicked);

        // SFX row  -- hoverStates[5], [6]
        UpdateVolumeRow(ROW_SFX, scale,
                        sfxVolume, &BGMManager::SetSFXVolume,
                        5, 6, clicked);

        // Close button  -- hoverStates[0]
        bool closeHov = Hovered(POP_CX, ROW_CLOSE, 235.f, 67.f, scale);
        if (closeHov && !hoverStates[0]) bgm.PlayUIHover();
        hoverStates[0] = closeHov;
        if (closeHov && clicked)
        {
            bgm.PlayUIClick();
            popupOpen = false;
        }

        // ESC closes without sound
        if (AEInputCheckTriggered(AEVK_ESCAPE))
            popupOpen = false;

        return true; // consumed this frame
    }

    void Draw(s8 font, s8 bigFont, float scale)
    {
        if (!popupOpen) return;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // Dim overlay
        DrawRect(DEFAULT_W / 2.f, DEFAULT_H / 2.f,
                 DEFAULT_W, DEFAULT_H, scale, 0.f, 0.f, 0.f, 0.55f);

        // Panel background
        DrawRect(POP_CX, POP_CY, POP_W, POP_H, scale, 0.25f, 0.25f, 0.25f);

        // Title bar
        DrawRect(POP_CX, ROW_TITLE, POP_W - 20.f, 70.f, scale, 0.75f, 0.75f, 0.75f);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        DrawAEText(bigFont, "SETTINGS",
                   D2W(POP_CX, ROW_TITLE, scale), scale * 0.65f,
                   CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        // Volume rows
        DrawVolumeRow(font, ROW_BGM, scale, "BGM", bgmVolume,
                      hoverStates[1], hoverStates[2]);
        DrawVolumeRow(font, ROW_UI,    scale, "UI",    uiVolume,
                      hoverStates[3], hoverStates[4]);
        DrawVolumeRow(font, ROW_SFX,   scale, "SFX",   sfxVolume,
                      hoverStates[5], hoverStates[6]);

        // Close button
        float cTint = hoverStates[0] ? 0.9f : 0.75f;
        DrawRect(POP_CX, ROW_CLOSE, 235.f, 67.f, scale, cTint, cTint, cTint);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        DrawAEText(font, "Close",
                   D2W(POP_CX, ROW_CLOSE, scale), scale,
                   CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    }

} // namespace Settings
