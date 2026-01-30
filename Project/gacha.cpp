#include "Gacha.h"
#include "AEEngine.h"
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <map>

// ============================================================
// Particle System
// ============================================================
struct GachaParticle {
    float x, y, vx, vy, life, r, g, b;
};

static std::vector<GachaParticle> s_particles;
static std::map<std::string, int> s_rarityCounts;

// ============================================================
// Visual State
// ============================================================
static float s_screenShake = 0.0f;

static bool  s_chestOpened = false;
static float s_chestLightTimer = 0.0f;
static const float s_chestLightMax = 0.85f;

static float s_chestBounceTimer = 0.0f;
static float s_chestBounceOffset = 0.0f;

static float s_idleGlowTimer = 0.0f;

static WordEntry s_highestEntry;

// ============================================================
// Solid Quad Mesh
// ============================================================
static AEGfxVertexList* s_overlayQuad = nullptr;

// ============================================================
// Gacha Pool
// ============================================================
static std::vector<WordEntry> gachaPool = {
    {"Fire",      "Common",     63,        1.0f, 1.0f, 1.0f},
    {"Nature",    "Uncommon",   20,        0.0f, 1.0f, 0.0f},
    {"Water",     "Rare",       10,        0.0f, 0.5f, 1.0f},
    {"Shadow",    "Epic",        5,        0.6f, 0.1f, 0.9f},
    {"Space",     "Legendary",   1.999999f,1.0f, 0.5f, 0.0f},
    {"Infinity",  "Mythical",    0.000001f,1.0f, 0.84f,0.0f}
};

// ============================================================
// Helpers
// ============================================================
static void SpawnBurst(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; ++i) {
        float a = (rand() % 360) * 0.0174533f;
        float s = (rand() % 100) / 1000.0f + 0.01f;
        s_particles.push_back({ x, y, cosf(a) * s, sinf(a) * s, 1.0f, r, g, b });
    }
}

WordEntry RollGachaWord() {
    float total = 0.0f;
    for (auto& e : gachaPool) total += e.weight;

    float r = ((float)rand() / RAND_MAX) * total;
    for (auto& e : gachaPool) {
        if (r < e.weight) return e;
        r -= e.weight;
    }
    return gachaPool.back();
}

static int RarityRank(const std::string& r) {
    if (r == "Common") return 0;
    if (r == "Uncommon") return 1;
    if (r == "Rare") return 2;
    if (r == "Epic") return 3;
    if (r == "Legendary") return 4;
    if (r == "Mythical") return 5;
    return -1;
}

static WordEntry FindHighestRarity(const std::vector<WordEntry>& r) {
    WordEntry best = r[0];
    for (auto& e : r)
        if (RarityRank(e.rarity) > RarityRank(best.rarity))
            best = e;
    return best;
}

// ============================================================
// Mesh Builder
// ============================================================
void EnsureOverlayMesh() {
    if (s_overlayQuad) return;

    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, -0.5f, 0xFFFFFFFF, 1, 0, 0.5f, 0.5f, 0xFFFFFFFF, 1, 1);
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, 0.5f, 0xFFFFFFFF, 1, 1, -0.5f, 0.5f, 0xFFFFFFFF, 0, 1);
    s_overlayQuad = AEGfxMeshEnd();
}

static void DrawSolidRect(const AEMtx33& parent,
    float x, float y, float w, float h,
    float r, float g, float b, float a)
{
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    AEMtx33 S, T, M;
    AEMtx33Scale(&S, w, h);
    AEMtx33Trans(&T, x, y);
    AEMtx33Concat(&M, &T, &S);
    AEMtx33Concat(&M, &parent, &M);

    AEGfxSetTransform(M.m);
    AEGfxMeshDraw(s_overlayQuad, AE_GFX_MDM_TRIANGLES);
}

// ============================================================
// Begin Overlay
// ============================================================
void BeginGachaOverlay(GachaAnimation& anim, int count, float intro, float roll, float delay) {
    anim.Reset();
    anim.currentIndex = -1; // start before first reveal

    s_particles.clear();
    s_rarityCounts.clear();

    s_chestOpened = false;
    s_chestLightTimer = 0.0f;
    s_chestBounceTimer = 0.0f;
    s_chestBounceOffset = 0.0f;
    s_idleGlowTimer = 0.0f;

    for (int i = 0; i < count; ++i) {
        auto r = RollGachaWord();
        anim.results.push_back(r);
        s_rarityCounts[r.rarity]++;
    }

    s_highestEntry = FindHighestRarity(anim.results);

    anim.phase = GachaPhase::Intro;
    anim.introTimer = intro;
    anim.rollingTimer = roll;
    anim.revealDelay = delay;
}

// ============================================================
// Update Overlay
// ============================================================
void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skip, bool open) {

    // Intro -> Rolling
    if (anim.phase == GachaPhase::Intro) {
        anim.introTimer -= dt;
        if (anim.introTimer <= 0.0f)
            anim.phase = GachaPhase::Rolling;
        return;
    }

    s_idleGlowTimer += dt;

    if (s_chestBounceTimer > 0.0f) {
        s_chestBounceTimer -= dt;
        float t = s_chestBounceTimer / 0.35f;
        s_chestBounceOffset = sinf(t * 3.14159f) * 0.08f;
    }
    else s_chestBounceOffset = 0.0f;

    // Open chest
    if (anim.phase == GachaPhase::Rolling && open && !s_chestOpened) {
        s_chestOpened = true;
        s_chestBounceTimer = 0.35f;
        s_chestLightTimer = s_chestLightMax;
        SpawnBurst(0, -0.9f, s_highestEntry.r, s_highestEntry.g, s_highestEntry.b, 260);
        s_screenShake = 0.45f;
    }

    // Chest light countdown
    if (s_chestOpened && anim.phase == GachaPhase::Rolling) {
        s_chestLightTimer -= dt;
        if (s_chestLightTimer <= 0.0f) {
            anim.phase = GachaPhase::Reveal;
            anim.timer = 0.0f;
        }
    }

    // Replace your Reveal phase logic with this safety check
    if (anim.phase == GachaPhase::Reveal) {
        anim.timer -= dt;
        if (anim.timer <= 0.0f) {
            // Increment first
            anim.currentIndex++;

            // SAFETY: Only access the vector if the index is valid
            if (anim.currentIndex >= 0 && anim.currentIndex < (int)anim.results.size()) {
                auto& e = anim.results[anim.currentIndex];

                // Trigger effects for high rarity
                if (RarityRank(e.rarity) >= 4) {
                    SpawnBurst(0, 0, e.r, e.g, e.b, 120);
                    s_screenShake = 0.35f;
                }
                else {
                    SpawnBurst(0, 0, e.r, e.g, e.b, 12);
                }
                anim.timer = anim.revealDelay;
            }

            // Check if we are finished
            if (anim.currentIndex >= (int)anim.results.size() - 1) {
                anim.phase = GachaPhase::Done;
                anim.isFinished = true;
            }
        }
    }

    // Update particles
    for (int i = 0; i < (int)s_particles.size();) {
        s_particles[i].x += s_particles[i].vx;
        s_particles[i].y += s_particles[i].vy;
        s_particles[i].life -= dt * 2.0f;
        if (s_particles[i].life <= 0) {
            s_particles[i] = s_particles.back();
            s_particles.pop_back();
        }
        else ++i;
    }

    if (s_screenShake > 0) s_screenShake -= dt * 2.0f;
}

// ============================================================
// Draw Overlay
// ============================================================
void DrawGachaOverlay(GachaAnimation& anim, s8 fontId) {
    EnsureOverlayMesh();
    AEGfxSetCamPosition(0, 0);
    AEMtx33 I; AEMtx33Identity(&I);

    float glow = 0.25f + 0.15f * sinf(s_idleGlowTimer * 2.0f);

    // Screen shake
    float shakeX = (s_screenShake > 0 ? ((rand() % 100) / 1000.0f - 0.05f) * s_screenShake * 10.0f : 0.0f);
    float shakeY = (s_screenShake > 0 ? ((rand() % 100) / 1000.0f - 0.05f) * s_screenShake * 10.0f : 0.0f);
    AEMtx33 shake; AEMtx33Trans(&shake, shakeX, shakeY);

    // Draw particles
    for (auto& p : s_particles)
        AEGfxPrint(fontId, ".", p.x, p.y, 1.2f, p.r, p.g, p.b, p.life);

    // Rolling chest
    if (anim.phase == GachaPhase::Rolling) {
        float w = AEGfxGetWinMaxX() - AEGfxGetWinMinX();
        float h = AEGfxGetWinMaxY() - AEGfxGetWinMinY();

        DrawSolidRect(I,
            0.0f,
            -1.0f + s_chestBounceOffset, // lower + bounce
            w * 0.30f, h * 0.22f,
            1.0f, 1.0f, 0.0f,
            s_chestOpened ? 1.0f : glow
        );

        if (!s_chestOpened)
            AEGfxPrint(fontId, "[O]: Open", -0.12f, -1.1f, 0.8f, 1, 1, 1, 1);

        // UI hints
        AEGfxPrint(fontId, "[SPACE]: Skip Animation | [R]: 10 Pull | [T]: 100 Pull | [ESC]: Quit", -0.7f, -0.9f, 0.6f, 1, 1, 1, 1);
        return;
    }

    // Reveal / Done
    if (anim.results.size() <= 10) {
        for (int i = 0; i <= anim.currentIndex; ++i) {
            auto& e = anim.results[i];
            float x = 0.0f, y = 0.0f, sc = 1.3f;
            if (i < 3) { x = -0.55f + i * 0.55f; y = 0.70f; }
            else if (i < 6) { x = -0.55f + (i - 3) * 0.55f; y = 0.40f; }
            else if (i < 9) { x = -0.55f + (i - 6) * 0.55f; y = 0.10f; }
            else { x = 0.0f; y = -0.20f; }

            float offset = -((float)e.word.length() * sc * 0.022f);
            AEGfxPrint(fontId, e.word.c_str(), x + offset, y, sc, e.r, e.g, e.b, 1.0f);
        }
    }
    else {
        for (int i = 0; i <= anim.currentIndex; i++) {
            int row = i / 10, col = i % 10;
            auto& e = anim.results[i];

            if (i == anim.currentIndex && (e.rarity == "Legendary" || e.rarity == "Mythical") && !anim.isFinished) {
                AEGfxPrint(fontId, "!!! NEW !!!", -0.12f, 0.25f, 1.0f, 1, 1, 0, 1);
                AEGfxPrint(fontId, e.word.c_str(), -0.25f, 0.0f, 4.0f, e.r, e.g, e.b, 1.0f);
                AEGfxPrint(fontId, e.rarity.c_str(), -0.15f, -0.2f, 1.2f, e.r, e.g, e.b, 1.0f);
            }
            else {
                AEGfxPrint(fontId, ".", -0.72f + (col * 0.16f), 0.65f - (row * 0.11f), 1.2f, e.r, e.g, e.b, 1.0f);
            }
        }

        if (anim.phase == GachaPhase::Done) {
            std::string s = "MYTHIC: " + std::to_string(s_rarityCounts["Mythical"]) + " | LEGEND: " + std::to_string(s_rarityCounts["Legendary"]);
            AEGfxPrint(fontId, s.c_str(), -0.42f, -0.65f, 0.8f, 1, 1, 0, 1);
        }
    }

    // UI hint text always
    AEGfxPrint(fontId, "[SPACE]: Skip Animation | [O]: Open | [R]: 10 Pull | [T]: 100 Pull | [ESC]: Quit", -0.7f, -0.9f, 0.6f, 1, 1, 1, 1);
}
