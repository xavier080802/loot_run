#include "CreditState.h"
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
    s8 Font = -1;

    constexpr float DEFAULT_W = 1600.0f;
    constexpr float DEFAULT_H = 900.0f;

    float winW, winH, scale;

    AEAudioGroup audioGroup;
    AEAudio clickSound;

    // --- State machine ---
    // WAITING  : black screen, flashing "Touch to reveal"
    // UNROLLING: scroll grows downward from centre
    // SCROLLING: credits roll upward inside the scroll
    enum ScrollState { STATE_WAITING, STATE_UNROLLING, STATE_SCROLLING };
    ScrollState state = STATE_WAITING;

    // --- Scroll geometry ---
    float scrollBodyHeight = 0.0f;
    const float SCROLL_FULL_HEIGHT = 680.0f;
    const float SCROLL_UNROLL_SPEED = 10.0f;
    const float SCROLL_W = 580.0f;
    const float CAP_H = 28.0f; // rolled curl height at top and bottom

    // --- Credits text ---
    float yPos_credits = 0.0f;
    const float LINE_H = 58.0f;
    const float SCROLL_SPEED = 1.0f;

    // Flash timer for "Touch to reveal"
    float flashTimer = 0.0f;

    // Converts default-resolution coords to world space, matching MainMenuState
    AEVec2 DefaultToWorld(float x, float y)
    {
        return {
            (x - DEFAULT_W * 0.5f) * scale,
            (DEFAULT_H * 0.5f - y) * scale
        };
    }

    const char* credits[] = {
        "Team STD::Null",
        " ",
        "Team Members:",
        " ",
        "TEAM LEAD and PROGRAMMER",
        "Xavier Lim",
        " ",
        "TECH LEAD and PROGRAMMER",
        "Edna",
        " ",
        "GAMEPLAY LEAD and PROGRAMMER",
        "Hong Teck",
        " ",
        "UI LEAD and PROGRAMMER",
        "Joon Hin",
        " ",
        "Faculty and Advisors",
        " ",
        "Instructors",
        "MR Gerald Wong",
        "MR Tommy Tan",
        "DR Soroor",
        " ",
        "SPECIAL THANKS TO",
        "Placeholder",
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
        "Mandy WONG   Johnny DEEK",
        " ",
        "Copyrights for software, tools and libraries",
        "(C) Kenney Assets",
        "(C) MetaDigger",
        nullptr
    };

    // Returns true if the line is all caps — treat as a section header
    bool IsHeader(const char* line)
    {
        if (!line || line[0] == ' ' || line[0] == '\0') return false;
        for (int i = 0; line[i] != '\0'; ++i)
            if (line[i] >= 'a' && line[i] <= 'z') return false;
        return true;
    }

    // Draws the parchment scroll using only AEGfx — no images needed.
    // cx/cy = world-space centre of the scroll, bodyH = current body height.
    void DrawScroll(float cx, float cy, float bodyH)
    {
        if (bodyH <= 0.0f) return;

        float left = cx - SCROLL_W * 0.5f * scale;
        float bodyTop = cy - bodyH * 0.5f;
        float scaledW = SCROLL_W * scale;

        AEMtx33 mtx;
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxTextureSet(nullptr, 0, 0);

        // --- Parchment body (warm cream) ---
        GetTransformMtx(mtx, { cx, cy }, 0.0f, { scaledW, bodyH });
        AEGfxSetTransform(mtx.m);
        AEGfxSetColorToMultiply(0.94f, 0.86f, 0.67f, 1.0f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // --- Left edge shadow strip ---
        float stripW = 18.0f * scale;
        GetTransformMtx(mtx, { left + stripW * 0.5f, cy }, 0.0f, { stripW, bodyH });
        AEGfxSetTransform(mtx.m);
        AEGfxSetColorToMultiply(0.70f, 0.61f, 0.39f, 0.47f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // --- Right edge shadow strip ---
        GetTransformMtx(mtx, { left + scaledW - stripW * 0.5f, cy }, 0.0f, { stripW, bodyH });
        AEGfxSetTransform(mtx.m);
        AEGfxSetColorToMultiply(0.70f, 0.61f, 0.39f, 0.47f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // --- Top curl (dark ellipse + lighter inner) ---
        float capScaledH = CAP_H * scale;
        // outer curl
        GetTransformMtx(mtx, { cx, bodyTop }, 0.0f, { scaledW, capScaledH });
        AEGfxSetTransform(mtx.m);
        AEGfxSetColorToMultiply(0.63f, 0.47f, 0.24f, 1.0f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
        // inner highlight
        GetTransformMtx(mtx, { cx, bodyTop }, 0.0f, { scaledW * 0.88f, capScaledH * 0.7f });
        AEGfxSetTransform(mtx.m);
        AEGfxSetColorToMultiply(0.78f, 0.63f, 0.35f, 1.0f);
        AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

        // --- Bottom curl — only once scroll is tall enough ---
        if (bodyH > CAP_H * 2.0f * scale) {
            float bottomY = bodyTop + bodyH;
            GetTransformMtx(mtx, { cx, bottomY }, 0.0f, { scaledW, capScaledH });
            AEGfxSetTransform(mtx.m);
            AEGfxSetColorToMultiply(0.63f, 0.47f, 0.24f, 1.0f);
            AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

            GetTransformMtx(mtx, { cx, bottomY }, 0.0f, { scaledW * 0.88f, capScaledH * 0.7f });
            AEGfxSetTransform(mtx.m);
            AEGfxSetColorToMultiply(0.78f, 0.63f, 0.35f, 1.0f);
            AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
        }
    }
}

// =============================================================
void CreditState::LoadState()
{
    // =============================================================
    squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
    Font = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 28);
    audioGroup = AEAudioCreateGroup();
    clickSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17766.wav");
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

    // --- STATE: WAITING ---
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

    // --- STATE: UNROLLING ---
    if (state == STATE_UNROLLING) {
        scrollBodyHeight += SCROLL_UNROLL_SPEED * scale;
        float fullH = SCROLL_FULL_HEIGHT * scale;
        if (scrollBodyHeight >= fullH) {
            scrollBodyHeight = fullH;
            state = STATE_SCROLLING;
            // Start credits just below the bottom of the scroll interior
            // so they scroll upward into view from the bottom
            float cy = 0.0f; // world centre
            yPos_credits = cy + fullH * 0.5f - CAP_H * scale;
        }
        return;
    }

    // --- STATE: SCROLLING ---
    if (state == STATE_SCROLLING) {
        yPos_credits -= SCROLL_SPEED * scale;
    }
}

// =============================================================
void CreditState::Draw()
{
    // =============================================================
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f); // black

    float cx = 0.0f; // world centre x
    float cy = 0.0f; // world centre y

    // -------------------------------------------------------
    // STATE: WAITING — flash "Touch to reveal"
    // -------------------------------------------------------
    if (state == STATE_WAITING) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        if ((int)(flashTimer / 0.6f) % 2 == 0) {
            DrawAEText(Font, "Touch to reveal", { cx, cy }, scale,
                CreateColor(200, 170, 80, 255), TEXT_MIDDLE);
        }
        // ESC hint bottom left
        AEVec2 hintPos = DefaultToWorld(80.0f, DEFAULT_H - 40.0f);
        DrawAEText(Font, "[ESC] Back", hintPos, scale * 0.75f,
            CreateColor(140, 140, 140, 255), TEXT_MIDDLE);
        return;
    }

    // -------------------------------------------------------
    // STATE: UNROLLING — draw scroll growing downward
    // -------------------------------------------------------
    if (state == STATE_UNROLLING) {
        DrawScroll(cx, cy, scrollBodyHeight);
        AEVec2 hintPos = DefaultToWorld(80.0f, DEFAULT_H - 40.0f);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        DrawAEText(Font, "[ESC] Back", hintPos, scale * 0.75f,
            CreateColor(140, 140, 140, 255), TEXT_MIDDLE);
        return;
    }

    // -------------------------------------------------------
    // STATE: SCROLLING — draw scroll + clipped rolling text
    // -------------------------------------------------------
    float fullH = SCROLL_FULL_HEIGHT * scale;
    float scaledW = SCROLL_W * scale;

    DrawScroll(cx, cy, fullH);

    // Clip region = scroll interior, inset slightly from the edge shadows
    float clipInset = 20.0f * scale;
    float clipX = (winW * 0.5f) - scaledW * 0.5f + clipInset;
    float clipY = (winH * 0.5f) - fullH * 0.5f + CAP_H * scale;
    float clipW = scaledW - clipInset * 2.0f;
    float clipH = fullH - CAP_H * scale * 2.0f;

    // AE doesn't have a built-in clip rect — we skip drawing lines outside the window manually
    float clipWorldTop = cy - fullH * 0.5f + CAP_H * scale;
    float clipWorldBottom = cy + fullH * 0.5f - CAP_H * scale;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

    int totalLines = 0;
    for (int i = 0; credits[i] != nullptr; ++i) totalLines++;

    for (int i = 0; credits[i] != nullptr; ++i) {
        float y = yPos_credits + i * LINE_H * scale;

        // skip lines outside the visible scroll interior
        if (y < clipWorldTop)    continue;
        if (y > clipWorldBottom) continue;

        if (IsHeader(credits[i])) {
            // Dark gold — readable on cream parchment
            DrawAEText(Font, credits[i], { cx, y }, scale * 0.85f,
                CreateColor(160, 110, 10, 255), TEXT_MIDDLE);
        }
        else {
            // Dark brown for names
            DrawAEText(Font, credits[i], { cx, y }, scale * 0.7f,
                CreateColor(40, 25, 10, 255), TEXT_MIDDLE);
        }
    }

    // Loop back when all lines scroll past the top of the scroll
    // When the last line scrolls above the top edge, loop back from the bottom
    if (yPos_credits + totalLines * LINE_H * scale < clipWorldTop) {
        yPos_credits = clipWorldBottom;
    }

    // ESC hint
    AEVec2 hintPos = DefaultToWorld(80.0f, DEFAULT_H - 40.0f);
    DrawAEText(Font, "[ESC] Back", hintPos, scale * 0.75f,
        CreateColor(140, 140, 140, 255), TEXT_MIDDLE);
}

// =============================================================
void CreditState::ExitState()
{
    // =============================================================
}

// =============================================================
void CreditState::UnloadState()
{
    // =============================================================
    if (Font >= 0) AEGfxDestroyFont(Font);
    AEAudioUnloadAudio(clickSound);
    AEAudioUnloadAudioGroup(audioGroup);
}