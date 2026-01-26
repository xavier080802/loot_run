#include "Gacha.h"
#include <cstdlib>
#include <cmath>
#include <vector>

// ---------------------------
// Particle System
// ---------------------------
struct GachaParticle {
    float x, y, vx, vy, life, r, g, b;
};
static std::vector<GachaParticle> s_particles;

static void SpawnBurst(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (rand() % 360) * (3.14159f / 180.0f);
        float speed = (rand() % 100) / 1000.0f + 0.005f;
        s_particles.push_back({ x, y, cosf(angle) * speed, sinf(angle) * speed, 1.0f, r, g, b });
    }
}

// ---------------------------
// Rarity Pool
// ---------------------------
static std::vector<WordEntry> gachaPool = {
    {"Fire",      "Common",    63, 1.0f, 1.0f, 1.0f}, // White
    {"Nature",    "Uncommon",  20, 0.0f, 1.0f, 0.0f}, // Green
    {"Water",     "Rare",      10, 0.0f, 0.5f, 1.0f}, // Blue
    {"Shadow",    "Epic",       5, 0.6f, 0.1f, 0.9f}, // Purple
    {"Space",     "Legendary",  1.999999, 1.0f, 0.0f, 0.0f}, // Red
    {"Infinity",  "Mythical",   0.000001, 1.0f, 0.84f, 0.0f} // Gold
};


WordEntry RollGachaWord() {
    int totalWeight = 0;
    for (const auto& entry : gachaPool) totalWeight += (int)entry.weight;
    if (totalWeight <= 0) return gachaPool[0];
    int roll = rand() % totalWeight;
    for (const auto& entry : gachaPool) {
        if (roll < entry.weight) return entry;
        roll -= (int)entry.weight;
    }
    return gachaPool.back();
}

std::vector<WordEntry> MultiRoll(int count) {
    std::vector<WordEntry> results;
    for (int i = 0; i < count; ++i) results.push_back(RollGachaWord());
    return results;
}

void UpdateGachaAnimation(GachaAnimation& anim, float dt) {
    if (anim.isFinished || anim.results.empty()) return;

    anim.timer -= dt;
    if (anim.timer <= 0.0f) {
        anim.currentIndex++;
        anim.timer = anim.revealDelay;

        // SPAWN PARTICLES ON REVEAL
        const auto& e = anim.results[anim.currentIndex];
        SpawnBurst(0.0f, 0.5f - (anim.currentIndex * 0.12f), e.r, e.g, e.b, 15);

        // --- INSERT SOUND MANAGER HERE ---
        // SoundManager->Play("reveal");

        if (anim.currentIndex >= (int)anim.results.size() - 1) anim.isFinished = true;
    }

    // Update particles
    for (int i = 0; i < (int)s_particles.size(); ++i) {
        s_particles[i].x += s_particles[i].vx;
        s_particles[i].y += s_particles[i].vy;
        s_particles[i].life -= dt * 1.5f;
        if (s_particles[i].life <= 0) {
            s_particles.erase(s_particles.begin() + i);
            i--;
        }
    }
}

static AEGfxVertexList* s_overlayMesh = nullptr;
void EnsureOverlayMesh() {
    if (s_overlayMesh) return;
    AEGfxMeshStart();
    AEGfxTriAdd(-1.0f, -1.0f, 0xFFFFFFFF, 0, 0, 1.0f, -1.0f, 0xFFFFFFFF, 0, 0, 1.0f, 1.0f, 0xFFFFFFFF, 0, 0);
    AEGfxTriAdd(-1.0f, -1.0f, 0xFFFFFFFF, 0, 0, 1.0f, 1.0f, 0xFFFFFFFF, 0, 0, -1.0f, 1.0f, 0xFFFFFFFF, 0, 0);
    s_overlayMesh = AEGfxMeshEnd();
}

void BeginGachaOverlay(GachaAnimation& anim, int rollCount, float introTime, float rollingTime, float revealDelay) {
    anim.Reset();
    s_particles.clear();
    anim.results = MultiRoll(rollCount);
    anim.phase = GachaPhase::Intro;
    anim.introTimer = introTime;
    anim.rollingTimer = rollingTime;
    anim.revealDelay = revealDelay;
}

void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skipPressed) {
    if (anim.phase == GachaPhase::None) return;
    anim.elapsed += dt;

    if (skipPressed && anim.phase != GachaPhase::Done) {
        anim.currentIndex = (int)anim.results.size() - 1;
        anim.isFinished = true;
        anim.phase = GachaPhase::Done;
    }

    switch (anim.phase) {
    case GachaPhase::Intro:
        anim.introTimer -= dt;
        if (anim.introTimer <= 0.0f) anim.phase = GachaPhase::Rolling;
        break;
    case GachaPhase::Rolling:
        anim.fakeTickTimer -= dt;
        if (anim.fakeTickTimer <= 0.0f) {
            anim.fakeSlots[1] = RollGachaWord();
            anim.fakeTickTimer = 0.06f;
            // SoundManager->Play("tick");
        }
        anim.rollingTimer -= dt;
        if (anim.rollingTimer <= 0.0f) anim.phase = GachaPhase::Reveal;
        break;
    case GachaPhase::Reveal:
        UpdateGachaAnimation(anim, dt);
        if (anim.isFinished) anim.phase = GachaPhase::Done;
        break;
    }
}

void DrawGachaOverlay(GachaAnimation& anim, s8 fontId) {
    EnsureOverlayMesh();
    AEMtx33 I; AEMtx33Identity(&I); AEGfxSetTransform(I.m);

    // 1. SOLID BLACK BACKGROUND
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetColorToMultiply(0, 0, 0, 1.0f);
    AEGfxMeshDraw(s_overlayMesh, AE_GFX_MDM_TRIANGLES);

    // 2. DRAW PARTICLES
    for (auto& p : s_particles) {
        AEGfxPrint(fontId, (const char*)"*", p.x, p.y, 1.0f, p.r, p.g, p.b, p.life);
    }

    // 3. DRAW CENTERED TEXT
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxPrint(fontId, (const char*)"SUMMONING", -0.12f, 0.8f, 1.2f, 1, 1, 1, 1);

    if (anim.phase == GachaPhase::Rolling) {
        const auto& s = anim.fakeSlots[1];
        AEGfxPrint(fontId, (const char*)s.word.c_str(), -0.05f, 0.0f, 2.0f, s.r, s.g, s.b, 1.0f);
    }
    else {
        for (int i = 0; i <= anim.currentIndex; ++i) {
            const auto& e = anim.results[i];
            float scale = (i == anim.currentIndex) ? 1.2f : 1.0f;
            if (e.rarity == "Mythical") scale *= (1.1f + 0.1f * sinf(anim.elapsed * 10.0f));
            AEGfxPrint(fontId, (const char*)e.word.c_str(), -0.05f, 0.5f - (i * 0.12f), scale, e.r, e.g, e.b, 1.0f);
        }
    }

    const char* hint = (anim.phase == GachaPhase::Done ? "Press F to exit" : "Press SPACE to skip");
    AEGfxPrint(fontId, (const char*)hint, -0.18f, -0.85f, 0.8f, 0.6f, 0.6f, 0.6f, 1.0f);
}