#include "CreditState.h"
#include "../Music.h"
#include "../GameStateManager.h"
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/CoordUtils.h"
#include "../Helpers/CollisionUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/ColorUtils.h"
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
    BGMManager creditsBGM;

    // Three stages the credits screen moves through in order:
    // WAITING   -> player sees closed scroll and a flashing prompt
    // UNROLLING -> scroll opens downward (animated)
    // SCROLLING -> text rolls upward inside the open scroll
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

    // -------------------------------------------------------
    // SCROLL SPRITE CLIP SETTINGS
    // The open scroll sprite is 35x75 pixels.
    // SPRITE_CAP_TOP / SPRITE_CAP_BOT = pixel height of each curl rod.
    // To push the TOP start line lower   -> increase SPRITE_CAP_TOP
    // To push the BOTTOM end line higher -> increase BOTTOM_PADDING or bottomSafety
    // -------------------------------------------------------
    const float SPRITE_H = 75.0f;
    const float SPRITE_CAP_TOP = 14.0f;
    const float SPRITE_CAP_BOT = 14.0f;
    float BOTTOM_PADDING = 87.0f;   // base extra gap at the bottom

    float yPos_credits = 0.0f;
    const float LINE_H = 58.0f;
    const float SCROLL_SPEED = 1.0f;

    float flashTimer = 0.0f;

    // -------------------------------------------------------
    // CREDITS CONTENT
    // Written TOP TO BOTTOM first entry appears at the top
    // of the scroll first as text scrolls upward.
    // All-caps lines are auto-coloured gold as section headers.
    // Use " " for blank spacing lines between sections.
    // Always keep nullptr as the very last entry.
    // -------------------------------------------------------
    const char* credits[] = {
        "Team STD::Null",
        " ",
        "Team Members:",
        " ",
        "Xavier Lim",
        "TEAM LEAD and PROGRAMMER",
        " ",
        "Edna Sim",
        "TECH LEAD and PROGRAMMER",
        " ",
        "Hong Teck",
        "GAMEPLAY LEAD and PROGRAMMER",
        " ",
        "Joon Hin",
        "UI LEAD and PROGRAMMER",
        " ",
        "Faculty and Advisors",
        " ",
        "Instructors",
        "MR Gerald Wong",
        "MR Tommy Tan",
        "DR Soroor",
        " ",
        "SPECIAL THANKS TO",
        "NParks falcon chicks cam",
        " ",
        "Created at",
        "DigiPen Institute of Technology Singapore",
        " ",
        "PRESIDENT",
        "Claude COMAIR",
        " ",
        "EXECUTIVES",
        "Claude COMAIR   CHU Jason Yeu Tat   Michael GATS",
        "TAN Chek Ming   Prasanna Kumar GHALI",
        "Mandy WONG      Johnny DEEK",
        " ",
        "Copyrights for software, tools and libraries",
        "(C) Kenney Assets",
        "(C) MetaDigger",
        nullptr
    };

    // Converts a 1600x900 layout position into world space
    AEVec2 DefaultToWorld(float x, float y) {
        return { (x - DEFAULT_W * 0.5f) * scale, (DEFAULT_H * 0.5f - y) * scale };
    }

    /**
     * @brief Returns true if a credits line should be drawn as a gold section header.
     *
     * Any line with no lowercase letters is treated as a header.
     * Blank lines and null are excluded.
     *
     * @param line  The credits string to check.
     * @return true if the line is all-caps, false otherwise.
     */
    bool IsHeader(const char* line) {
        if (!line || line[0] == ' ' || line[0] == '\0') return false;
        for (int i = 0; line[i] != '\0'; ++i)
            if (line[i] >= 'a' && line[i] <= 'z') return false;
        return true;
    }

    /**
     * @brief Draws the open scroll sprite stretched to the given height.
     *
     * During the unroll animation bodyH grows each frame so the sprite
     * appears to unroll downward. Once fully open it stays at SCROLL_FULL_HEIGHT.
     *
     * @param cx     World-space centre X. Passed by VALUE.
     * @param cy     World-space centre Y. Passed by VALUE.
     * @param bodyH  Current draw height in world units. Passed by VALUE.
     */
    void DrawScroll(float cx, float cy, float bodyH) {
        if (bodyH <= 0.0f) return;
        AEVec2 pos = { cx, cy };
        AEVec2 size = { SCROLL_W * scale, bodyH };
        DrawTintedMesh(GetTransformMtx(pos, 0.0f, size), squareMesh, texOpen, { 255, 255, 255, 255 }, 255);
    }

    void DrawScrollClosed(float cx, float cy)
    {
        AEVec2 pos = { cx, cy };
        AEVec2 size = { SCROLL_W * scale, (SCROLL_W * 0.5f) * scale };
        DrawTintedMesh(GetTransformMtx(pos, 0.0f, size), squareMesh, texClosed, { 255, 255, 255, 255 }, 255);
    }
}

// =============================================================
void CreditState::LoadState()
{
    // =============================================================
    squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
    Font = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 28);
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

    // WAITING � tick flash timer, start unroll on any input
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

    // UNROLLING grow scroll downward until fully open, then start scrolling
    if (state == STATE_UNROLLING) {
        scrollBodyHeight += SCROLL_UNROLL_SPEED * scale;
        float fullH = SCROLL_FULL_HEIGHT * scale;
        if (scrollBodyHeight >= fullH) {
            scrollBodyHeight = fullH;
            state = STATE_SCROLLING;
            float cy = 0.0f;
            yPos_credits = cy + fullH * 0.5f - CAP_H * scale;
        }
        return;
    }

    // SCROLLING � move text upward each frame
    if (state == STATE_SCROLLING) {
        yPos_credits -= SCROLL_SPEED * scale;
    }
}

void CreditState::Draw()
{
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    float cx = 0.0f, cy = 0.0f;

    // WAITING � closed scroll + flashing prompt
    if (state == STATE_WAITING) {
        DrawScrollClosed(cx, cy);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        if ((int)(flashTimer / 0.6f) % 2 == 0) {
            AEVec2 promptPos = { cx, cy - SCROLL_W * 0.3f * scale };
            DrawAEText(Font, "Touch to reveal", promptPos, scale,
                CreateColor(200, 170, 80, 255), TEXT_MIDDLE);
        }
    }
    else {
        float fullH = SCROLL_FULL_HEIGHT * scale;

        // UNROLLING � draw growing scroll only, no text yet
        // SCROLLING � draw fully open scroll then clip and draw text
        DrawScroll(cx, cy, (state == STATE_UNROLLING) ? scrollBodyHeight : fullH);

    float fullH = SCROLL_FULL_HEIGHT * scale;
    float scaledW = SCROLL_W * scale;

    DrawScroll(cx, cy, fullH);
    float clipWorldTop = cy - fullH * 0.5f + SCROLL_FULL_HEIGHT * (SPRITE_CAP_TOP / SPRITE_H) * scale;
    float clipWorldBottom = cy + fullH * 0.5f - SCROLL_FULL_HEIGHT * (SPRITE_CAP_BOT / SPRITE_H) * scale - BOTTOM_PADDING;

            // Draw each line, skipping anything outside the parchment area
            for (int i = 0; i < totalLines; ++i) {
                float y = yPos_credits + (i * LINE_H * scale);

                if (y < clipWorldTop || y > clipWorldBottom) continue;

    for (int i = totalLines - 1; i >= 0; --i) {
        int   reversedIdx = (totalLines - 1) - i;
        float y = yPos_credits + reversedIdx * LINE_H * scale;

            // Once the last line scrolls above the top edge, loop back to the bottom
            float lastLineY = yPos_credits + ((totalLines - 1) * LINE_H * scale);
            if (lastLineY < clipWorldTop) {
                yPos_credits = clipWorldBottom;
            }
        }
    }

    // ESC hint  always visible in the bottom corner
    AEVec2 hintPos = DefaultToWorld(80.0f, DEFAULT_H - 40.0f);
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
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
    creditsBGM.Exit();
}