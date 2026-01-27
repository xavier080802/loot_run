#include "Gacha.h"
#include "AEEngine.h"
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <map>

struct GachaParticle {
    float x, y, vx, vy, life, r, g, b;
};

static std::vector<GachaParticle> s_particles;
static std::map<std::string, int> s_rarityCounts;
static float s_screenShake = 0.0f;

static void SpawnBurst(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (rand() % 360) * (3.14159f / 180.0f);
        float speed = (rand() % 100) / 1000.0f + 0.01f;
        s_particles.push_back({ x, y, cosf(angle) * speed, sinf(angle) * speed, 1.0f, r, g, b });
    }
}

static std::vector<WordEntry> gachaPool = {
    {"Fire",      "Common",    63, 1.0f, 1.0f, 1.0f},
    {"Nature",    "Uncommon",  20, 0.0f, 1.0f, 0.0f},
    {"Water",     "Rare",      10, 0.0f, 0.5f, 1.0f},
    {"Shadow",    "Epic",       5, 0.6f, 0.1f, 0.9f},
    {"Space",     "Legendary",  1.999999f, 1.0f, 0.5f, 0.0f},
    {"Infinity",  "Mythical",   0.000001f, 1.0f, 0.84f, 0.0f}
};

WordEntry RollGachaWord() {
    float totalWeight = 0;
    for (const auto& entry : gachaPool) totalWeight += entry.weight;
    float roll = (float)rand() / (float)RAND_MAX * totalWeight;
    for (const auto& entry : gachaPool) {
        if (roll < entry.weight) return entry;
        roll -= entry.weight;
    }
    return gachaPool.back();
}

void UpdateGachaAnimation(GachaAnimation& anim, float dt) {
    if (s_screenShake > 0) s_screenShake -= dt * 2.0f;

    for (int i = 0; i < (int)s_particles.size(); ) {
        s_particles[i].x += s_particles[i].vx;
        s_particles[i].y += s_particles[i].vy;
        s_particles[i].life -= dt * 2.0f;
        if (s_particles[i].life <= 0) {
            s_particles[i] = s_particles.back();
            s_particles.pop_back();
        }
        else { i++; }
    }

    if (anim.isFinished) return;

    anim.timer -= dt;
    if (anim.timer <= 0.0f) {
        anim.currentIndex++;
        float currentDelay = (anim.results.size() > 10) ? 0.02f : anim.revealDelay;
        anim.timer = currentDelay;

        if (anim.currentIndex < (int)anim.results.size()) {
            const auto& e = anim.results[anim.currentIndex];

            if (e.rarity == "Legendary" || e.rarity == "Mythical") {
                SpawnBurst(0.0f, 0.0f, e.r, e.g, e.b, 120);
                s_screenShake = 0.35f;
                anim.timer = 1.3f;
            }
            else if (anim.results.size() <= 10) {
                SpawnBurst(0.0f, 0.55f - (anim.currentIndex * 0.12f), e.r, e.g, e.b, 12);
            }
        }

        if (anim.currentIndex >= (int)anim.results.size() - 1) {
            anim.isFinished = true;
            anim.phase = GachaPhase::Done;
        }
    }
}

void BeginGachaOverlay(GachaAnimation& anim, int rollCount, float introTime, float rollingTime, float revealDelay) {
    anim.Reset();
    anim.isFinished = false;
    s_particles.clear();
    s_rarityCounts.clear();
    s_screenShake = 0.0f;

    for (int i = 0; i < rollCount; ++i) {
        WordEntry res = RollGachaWord();
        anim.results.push_back(res);
        s_rarityCounts[res.rarity]++;
    }

    anim.phase = GachaPhase::Intro;
    anim.introTimer = introTime;
    anim.rollingTimer = rollingTime;
    anim.revealDelay = revealDelay;
}

void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skipPressed) {
    if (anim.phase == GachaPhase::None) return;
    anim.elapsed += dt;

    if (skipPressed && anim.phase != GachaPhase::Done) {
        bool highRarityFound = false;
        for (int i = anim.currentIndex + 1; i < (int)anim.results.size(); ++i) {
            if (anim.results[i].rarity == "Legendary" || anim.results[i].rarity == "Mythical") {
                anim.currentIndex = i - 1;
                highRarityFound = true;
                break;
            }
        }
        if (!highRarityFound) {
            anim.currentIndex = (int)anim.results.size() - 1;
            anim.isFinished = true;
            anim.phase = GachaPhase::Done;
        }
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
            anim.fakeTickTimer = 0.05f;
        }
        anim.rollingTimer -= dt;
        if (anim.rollingTimer <= 0.0f) anim.phase = GachaPhase::Reveal;
        break;
    case GachaPhase::Reveal:
    case GachaPhase::Done:
        UpdateGachaAnimation(anim, dt);
        break;
    }
}

void DrawGachaOverlay(GachaAnimation& anim, s8 fontId) {
    // Screen Shake
    float oX = (s_screenShake > 0) ? ((rand() % 100) / 1000.0f - 0.05f) * s_screenShake * 10.0f : 0.0f;
    float oY = (s_screenShake > 0) ? ((rand() % 100) / 1000.0f - 0.05f) * s_screenShake * 10.0f : 0.0f;

    AEMtx33 trans; AEMtx33Trans(&trans, oX, oY); AEGfxSetTransform(trans.m);

    for (auto& p : s_particles)
        AEGfxPrint(fontId, ".", p.x, p.y, 1.2f, p.r, p.g, p.b, p.life);

    if (anim.phase == GachaPhase::Rolling) {
        const auto& s = anim.fakeSlots[1];
        AEGfxPrint(fontId, s.word.c_str(), -0.1f, 0.0f, 3.0f, s.r, s.g, s.b, 1.0f);
    }
    else {
        if (anim.results.size() <= 10) {
            for (int i = 0; i <= anim.currentIndex; ++i) {
                const auto& e = anim.results[i];
                float scale = (i == anim.currentIndex) ? 1.4f : 1.0f;
                AEGfxPrint(fontId, e.word.c_str(), -0.1f, 0.55f - (i * 0.12f), scale, e.r, e.g, e.b, 1.0f);
            }
        }
        else {
            // 100-pull Grid
            for (int i = 0; i <= anim.currentIndex; ++i) {
                int row = i / 10; int col = i % 10;
                const auto& e = anim.results[i];

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
    }

    const char* h;
    float startX;
    if (anim.phase == GachaPhase::Done) {
        h = "[R]: 10 Pull | [T]: 100 Pull | [ESC]: Quit";
        startX = -0.45f;
    }
    else {
        h = "[SPACE]: Skip Animation";
        startX = -0.25f;
    }
    AEGfxPrint(fontId, h, startX, -0.9f, 0.8f, 1, 1, 1, 1);
}

void EnsureOverlayMesh() {}