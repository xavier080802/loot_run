#include "CreditState.h"
#include "../GameStateManager.h"
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/CoordUtils.h"
#include "../Helpers/CollisionUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/ColorUtils.h"
#include "../UIConfig.h"
#include <iostream>

namespace {
    AEGfxVertexList* squareMesh = nullptr;
    AEGfxTexture* texClosed = nullptr;
    AEGfxTexture* texOpen = nullptr;
    s8 Font = -1;

    constexpr float DEFAULT_W = 1600.0f;
    constexpr float DEFAULT_H = 900.0f;

    float winW, winH, scale;

    AEAudioGroup audioGroup;
    AEAudio clickSound;
    AEAudio bgMusic;

    enum ScrollState { STATE_WAITING, STATE_UNROLLING, STATE_SCROLLING };
    ScrollState state = STATE_WAITING;

    float scrollBodyHeight = 0.0f;
    const float SCROLL_FULL_HEIGHT = 680.0f;
    const float SCROLL_UNROLL_SPEED = 10.0f;
    const float SCROLL_W = 580.0f;

    // Sprite is 35x75px.
    // SPRITE_CAP_TOP  — pixels from top of sprite to where parchment starts
    // SPRITE_CAP_BOT  — pixels from bottom of sprite to where parchment ends
    // Only edit these two numbers to move the clip boundaries independently.
    const float SPRITE_H = 75.0f;
    const float SPRITE_CAP_TOP = 14.0f;
    const float SPRITE_CAP_BOT = 14.0f;
    // Extra world-unit padding pulled inward from the bottom clip — increase to
    // make text disappear higher, decrease to let it go lower.
    const float BOTTOM_PADDING = 50.0f;

    const float CAP_H = SCROLL_FULL_HEIGHT * (SPRITE_CAP_TOP / SPRITE_H);

    float yPos_credits = 0.0f;
    const float LINE_H = 58.0f;
    const float SCROLL_SPEED = 1.0f;

    float flashTimer = 0.0f;

    AEVec2 DefaultToWorld(float x, float y)
    {
        return {
            (x - DEFAULT_W * 0.5f) * scale,
            (DEFAULT_H * 0.5f - y) * scale
        };
    }

    const char* credits[] = {
       "(C) MetaDigger",
       "(C) Kenney Assets",
       "Copyrights for software, tools and libraries",
       " ",
       "Mandy WONG   Johnny DEEK",
       "TAN Chek Ming   Prasanna Kumar GHALI",
       "Claude COMAIR   CHU Jason Yeu Tat   Michael GATS",
       "EXECUTIVES",
       " ",
       "Claude COMAIR",
       "PRESIDENT",
       " ",
       "DigiPen Institute of Technology Singapore",
       "Created at",
       " ",
       "NParks falcon chicks cam",
       "SPECIAL THANKS TO",
       " ",
       "DR Soroor Malekmohammadi Faradounbeh",
       "MR Tan Chee Wei, Tommy",
       "MR Wong Han Feng, Gerald",
       "Instructors",
       " ",
       "Faculty and Advisors",
       " ",
       "Joon Hin",
       "UI LEAD and PROGRAMMER",
       " ",
       "Hong Teck",
       "GAMEPLAY LEAD and PROGRAMMER",
       " ",
       "Edna Sim",
       "TECH LEAD and PROGRAMMER",
       " ",
       "Xavier Lim",
       "TEAM LEAD and PROGRAMMER",
       " ",
       "Team Members:",
       " ",
       "Team STD::Null",
       nullptr
    };

    bool IsHeader(const char* line)
    {
        if (!line || line[0] == ' ' || line[0] == '\0') return false;
        for (int i = 0; line[i] != '\0'; ++i)
            if (line[i] >= 'a' && line[i] <= 'z') return false;
        return true;
    }

    void DrawScroll(float cx, float cy, float bodyH)
    {
        if (bodyH <= 0.0f) return;
        AEVec2 pos = { cx, cy };
        AEVec2 size = { SCROLL_W * scale, bodyH };
        DrawTintedMesh(GetTransformMtx(pos, 0.0f, size),
            squareMesh, texOpen, { 255, 255, 255, 255 }, 255);
    }

    void DrawScrollClosed(float cx, float cy)
    {
        AEVec2 pos = { cx, cy };
        AEVec2 size = { SCROLL_W * scale, (SCROLL_W * 0.5f) * scale };
        DrawTintedMesh(GetTransformMtx(pos, 0.0f, size),
            squareMesh, texClosed, { 255, 255, 255, 255 }, 255);
    }
}

// =============================================================
void CreditState::LoadState()
{
    // =============================================================
    squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
    Font = AEGfxCreateFont(PRIMARY_FONT_PATH, 28);
    texClosed = RenderingManager::GetInstance()->LoadTexture("Assets/scroll_closed.png");
    texOpen = RenderingManager::GetInstance()->LoadTexture("Assets/scroll_open.png");
    std::cout << "[CreditState] scroll_closed: " << (texClosed ? "OK" : "FAILED") << "\n";
    std::cout << "[CreditState] scroll_open:   " << (texOpen ? "OK" : "FAILED") << "\n";
    audioGroup = AEAudioCreateGroup();
    clickSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17766.wav");
    bgMusic = AEAudioLoadSound("Assets/Audio/PROSPECTUS - Corporate MSCCRP1_50.wav");
}

// =============================================================
void CreditState::InitState()
{
    // =============================================================
    winW = static_cast<float>(AEGfxGetWinMaxX());
    winH = static_cast<float>(AEGfxGetWinMaxY());
    scale = (winW * 2.0f / DEFAULT_W) < (winH * 2.0f / DEFAULT_H)
        ? (winW * 2.0f / DEFAULT_W) : (winH * 2.0f / DEFAULT_H);

    state = STATE_WAITING;
    scrollBodyHeight = 0.0f;
    flashTimer = 0.0f;
    yPos_credits = 0.0f;

    AEAudioPlay(bgMusic, audioGroup, 1.0f, 1.0f, -1);
}

// =============================================================
void CreditState::Update(double dt)
{
    // =============================================================
    (void)dt;

    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
        return;
    }

    if (state == STATE_WAITING) {
        flashTimer += static_cast<float>(dt);
        if (AEInputCheckTriggered(AEVK_LBUTTON) ||
            AEInputCheckTriggered(AEVK_SPACE) ||
            AEInputCheckTriggered(AEVK_RETURN)) {
            AEAudioPlay(clickSound, audioGroup, 0.6f, 0.6f, 0);
            state = STATE_UNROLLING;
            scrollBodyHeight = 0.0f;
        }
        return;
    }

    if (state == STATE_UNROLLING) {
        scrollBodyHeight += SCROLL_UNROLL_SPEED * scale;
        float fullH = SCROLL_FULL_HEIGHT * scale;
        if (scrollBodyHeight >= fullH) {
            scrollBodyHeight = fullH;
            state = STATE_SCROLLING;
            int totalLines = 0;
            for (int i = 0; credits[i] != nullptr; ++i) totalLines++;
            float cy = 0.0f;
            yPos_credits = cy - fullH * 0.5f + CAP_H * scale + totalLines * LINE_H * scale;
        }
        return;
    }

    if (state == STATE_SCROLLING) {
        yPos_credits -= SCROLL_SPEED * scale;
    }
}

void CreditState::Draw()
{
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    float cx = 0.0f;
    float cy = 0.0f;

    if (state == STATE_WAITING) {
        DrawScrollClosed(cx, cy);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        if ((int)(flashTimer / 0.6f) % 2 == 0) {
            AEVec2 promptPos = { cx, cy - SCROLL_W * 0.3f * scale };
            DrawAEText(Font, "Touch to reveal", promptPos, scale,
                CreateColor(200, 170, 80, 255), TEXT_MIDDLE);
        }
        AEVec2 hintPos = DefaultToWorld(80.0f, DEFAULT_H - 40.0f);
        DrawAEText(Font, "[ESC] Back", hintPos, scale * 0.75f,
            CreateColor(140, 140, 140, 255), TEXT_MIDDLE);
        return;
    }

    if (state == STATE_UNROLLING) {
        DrawScroll(cx, cy, scrollBodyHeight);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEVec2 hintPos = DefaultToWorld(80.0f, DEFAULT_H - 40.0f);
        DrawAEText(Font, "[ESC] Back", hintPos, scale * 0.75f,
            CreateColor(140, 140, 140, 255), TEXT_MIDDLE);
        return;
    }

    float fullH = SCROLL_FULL_HEIGHT * scale;
    float scaledW = SCROLL_W * scale;

    DrawScroll(cx, cy, fullH);
    float clipWorldTop = cy - fullH * 0.5f + SCROLL_FULL_HEIGHT * (SPRITE_CAP_TOP / SPRITE_H) * scale;
    float clipWorldBottom = cy + fullH * 0.5f - SCROLL_FULL_HEIGHT * (SPRITE_CAP_BOT / SPRITE_H) * scale - BOTTOM_PADDING;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    int totalLines = 0;
    for (int i = 0; credits[i] != nullptr; ++i) totalLines++;

    for (int i = 0; i < totalLines; ++i) {
        float y = yPos_credits - i * LINE_H * scale;  // subtract so lines go upward

        if (y < clipWorldTop)    continue;
        if (y > clipWorldBottom) continue;

        if (IsHeader(credits[i])) {
            DrawAEText(Font, credits[i], { cx, y }, scale * 0.9f,
                CreateColor(160, 110, 10, 255), TEXT_MIDDLE);
        }
        else {
            DrawAEText(Font, credits[i], { cx, y }, scale * 0.72f,
                CreateColor(40, 25, 10, 255), TEXT_MIDDLE);
        }
    }

    if (yPos_credits - totalLines * LINE_H * scale > clipWorldBottom) {
        int totalLinesReset = totalLines;
        yPos_credits = clipWorldBottom + totalLinesReset * LINE_H * scale;
    }

    AEVec2 hintPos = DefaultToWorld(80.0f, DEFAULT_H - 40.0f);
    DrawAEText(Font, "[ESC] Back", hintPos, scale * 0.75f,
        CreateColor(140, 140, 140, 255), TEXT_MIDDLE);
}

void CreditState::ExitState()
{
    AEAudioStopGroup(audioGroup);
}

void CreditState::UnloadState()
{
    if (Font >= 0) AEGfxDestroyFont(Font);
    AEAudioUnloadAudio(clickSound);
    AEAudioUnloadAudio(bgMusic);
    AEAudioUnloadAudioGroup(audioGroup);
}