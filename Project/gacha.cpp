
#include "Gacha.h"
#include <cstdlib>
#include <cmath>

// ---------------------------
// Pool of possible rewards
// ---------------------------
static std::vector<WordEntry> gachaPool = {
    {"Fire",  "Common", 70, 1.0f, 1.0f, 1.0f},
    {"Water", "Rare",   20, 0.0f, 0.5f, 1.0f},
    {"Light", "Epic",   10, 1.0f, 0.84f, 0.0f}
};

// ---------------------------
// Weighted roll
// ---------------------------
WordEntry RollGachaWord() {
    int totalWeight = 0;
    for (const auto& entry : gachaPool) totalWeight += entry.weight;

    int roll = rand() % totalWeight;
    for (const auto& entry : gachaPool) {
        if (roll < entry.weight) return entry;
        roll -= entry.weight;
    }
    return gachaPool.back();
}

std::vector<WordEntry> MultiRoll(int count) {
    std::vector<WordEntry> results;
    results.reserve((size_t)count);
    for (int i = 0; i < count; ++i)
        results.push_back(RollGachaWord());
    return results;
}

// ---------------------------
// Reveal update (unchanged)
// ---------------------------
void UpdateGachaAnimation(GachaAnimation& anim, float dt) {
    if (anim.isFinished || anim.results.empty()) return;

    anim.timer -= dt;
    if (anim.timer <= 0.0f) {
        anim.currentIndex++;
        anim.timer = anim.revealDelay;

        if (anim.currentIndex >= (int)anim.results.size() - 1) {
            anim.currentIndex = (int)anim.results.size() - 1;
            anim.isFinished = true;
        }
    }
}

// ---------------------------
// Reveal draw (unchanged)
// ---------------------------
void DrawGachaAnimation(GachaAnimation& anim, s8 fontId) {
    if (anim.currentIndex < 0) return;

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    float xPos = -0.5f;
    float yPos = 0.5f;
    float scale = 1.0f;
    float alpha = 1.0f;

    for (int i = 0; i <= anim.currentIndex; ++i) {
        const auto& entry = anim.results[i];

        float xOff = 0.0f;
        if (entry.rarity == "Epic") {
            xOff = sinf(anim.elapsed * 25.0f + i * 3.7f) * 0.02f;
        }

        AEGfxPrint(fontId, entry.word.c_str(),
            xPos + xOff,
            yPos - (i * 0.12f),
            scale,
            entry.r, entry.g, entry.b, alpha);
    }
}

// =====================================================
// Overlay (NEW): full-screen dim + intro + slot + skip
// =====================================================
static AEGfxVertexList* s_overlayMesh = nullptr;

static void EnsureOverlayMesh() {
    if (s_overlayMesh) return;

    AEGfxMeshStart();
    AEGfxTriAdd(-1.0f, -1.0f, 0xFFFFFFFF, 0, 0, 1.0f, -1.0f, 0xFFFFFFFF, 0, 0, 1.0f, 1.0f, 0xFFFFFFFF, 0, 0);
    AEGfxTriAdd(-1.0f, -1.0f, 0xFFFFFFFF, 0, 0, 1.0f, 1.0f, 0xFFFFFFFF, 0, 0, -1.0f, 1.0f, 0xFFFFFFFF, 0, 0);
    s_overlayMesh = AEGfxMeshEnd();
}

void BeginGachaOverlay(GachaAnimation& anim, int rollCount,
    float introTime, float rollingTime, float revealDelay)
{
    anim.Reset();
    anim.results = MultiRoll(rollCount);
    anim.revealDelay = revealDelay;

    anim.phase = GachaPhase::Intro;
    anim.introTimer = introTime;
    anim.rollingTimer = rollingTime;

    anim.overlayAlpha = 0.0f;

    // init slot words
    anim.fakeSlots[0] = RollGachaWord();
    anim.fakeSlots[1] = RollGachaWord();
    anim.fakeSlots[2] = RollGachaWord();
    anim.fakeTickTimer = 0.0f;

    // reveal init
    anim.currentIndex = -1;
    anim.timer = anim.revealDelay;
    anim.isFinished = anim.results.empty();
}

static void JumpToReveal(GachaAnimation& anim) {
    anim.phase = GachaPhase::Reveal;
    anim.timer = 0.0f;          // reveal first immediately
    anim.currentIndex = -1;
    anim.isFinished = false;
}

static void SkipToDone(GachaAnimation& anim) {
    if (anim.results.empty()) {
        anim.phase = GachaPhase::Done;
        anim.isFinished = true;
        anim.currentIndex = -1;
        return;
    }
    anim.currentIndex = (int)anim.results.size() - 1;
    anim.isFinished = true;
    anim.phase = GachaPhase::Done;
}

void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skipPressed)
{
    if (anim.phase == GachaPhase::None) return;

    anim.elapsed += dt;

    // fade overlay in to ~0.75 alpha
    const float targetAlpha = 0.75f;
    const float fadeSpeed = 3.0f;
    if (anim.overlayAlpha < targetAlpha) {
        anim.overlayAlpha += fadeSpeed * dt;
        if (anim.overlayAlpha > targetAlpha) anim.overlayAlpha = targetAlpha;
    }

    // SKIP behavior
    if (skipPressed) {
        if (anim.phase == GachaPhase::Intro || anim.phase == GachaPhase::Rolling) {
            JumpToReveal(anim);
        }
        else if (anim.phase == GachaPhase::Reveal) {
            SkipToDone(anim);
        }
        // Done phase: let GameState close with F (your rule)
    }

    switch (anim.phase)
    {
    case GachaPhase::Intro:
    {
        anim.introTimer -= dt;
        if (anim.introTimer <= 0.0f) {
            anim.phase = GachaPhase::Rolling;
        }
    } break;

    case GachaPhase::Rolling:
    {
        // Update slot words quickly (slot-machine tick)
        anim.fakeTickTimer -= dt;
        if (anim.fakeTickTimer <= 0.0f) {
            // shift downward: top <- mid, mid <- bot, bot <- new
            anim.fakeSlots[0] = anim.fakeSlots[1];
            anim.fakeSlots[1] = anim.fakeSlots[2];
            anim.fakeSlots[2] = RollGachaWord();

            anim.fakeTickTimer = 0.06f; // slot speed
        }

        anim.rollingTimer -= dt;
        if (anim.rollingTimer <= 0.0f) {
            JumpToReveal(anim);
        }
    } break;

    case GachaPhase::Reveal:
    {
        UpdateGachaAnimation(anim, dt);
        if (anim.isFinished) {
            anim.phase = GachaPhase::Done;
        }
    } break;

    case GachaPhase::Done:
    default:
        break;
    }
}

void DrawGachaOverlay(GachaAnimation& anim, s8 fontId)
{
    if (anim.phase == GachaPhase::None) return;

    EnsureOverlayMesh();

    // Reset transform
    AEMtx33 I;
    AEMtx33Identity(&I);
    AEGfxSetTransform(I.m);

    // Draw dark overlay
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, anim.overlayAlpha);
    AEGfxMeshDraw(s_overlayMesh, AE_GFX_MDM_TRIANGLES);

    // UI text
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // Title
    AEGfxPrint(fontId, "Summoning", -0.18f, 0.78f, 1.2f, 1, 1, 1, 1);

    // Dots animation
    const char* dots = ((int)(anim.elapsed * 3) % 3 == 0) ? "." :
        ((int)(anim.elapsed * 3) % 3 == 1) ? ".." : "...";
    AEGfxPrint(fontId, dots, 0.13f, 0.78f, 1.2f, 1, 1, 1, 1);

    // Skip hint (shown before Done)
    if (anim.phase != GachaPhase::Done) {
        AEGfxPrint(fontId, "Press SPACE to skip", -0.25f, -0.85f, 0.9f, 1, 1, 1, 1);
    }

    if (anim.phase == GachaPhase::Intro)
    {
        // small center prompt
        AEGfxPrint(fontId, "Get ready...", -0.12f, 0.15f, 1.0f, 1, 1, 1, 1);
        return;
    }

    if (anim.phase == GachaPhase::Rolling)
    {
        // SLOT MACHINE: 3 stacked words, center highlighted
        float cx = -0.06f;      // center-ish
        float cy = 0.25f;

        // top (dim)
        const auto& t = anim.fakeSlots[0];
        AEGfxPrint(fontId, t.word.c_str(), cx, cy + 0.18f, 1.4f, t.r, t.g, t.b, 0.45f);

        // middle (highlight)
        const auto& m = anim.fakeSlots[1];
        // slight pulse for highlight
        float pulse = 1.9f + 0.12f * sinf(anim.elapsed * 10.0f);
        AEGfxPrint(fontId, m.word.c_str(), cx, cy, pulse, m.r, m.g, m.b, 1.0f);

        // bottom (dim)
        const auto& b = anim.fakeSlots[2];
        AEGfxPrint(fontId, b.word.c_str(), cx, cy - 0.18f, 1.4f, b.r, b.g, b.b, 0.45f);

        // optional "Rolling..."
        AEGfxPrint(fontId, "Rolling...", -0.10f, 0.02f, 1.0f, 1, 1, 1, 1);
        return;
    }

    // Reveal + Done: show results list on left like your screenshot
    DrawGachaAnimation(anim, fontId);

    if (anim.phase == GachaPhase::Done) {
        AEGfxPrint(fontId, "Press F to close", -0.20f, -0.85f, 0.9f, 1, 1, 1, 1);
    }
}
