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
static AEGfxVertexList* s_overlayQuad = nullptr;

static bool  s_chestOpened = false;
static float s_chestLightTimer = 0.0f;
static const float s_chestLightMax = 0.85f; 
static WordEntry s_highestEntry;           


static std::vector<WordEntry> gachaPool = {
    {"Fire",      "Common",     63,        1.0f, 1.0f, 1.0f},
    {"Nature",    "Uncommon",   20,        0.0f, 1.0f, 0.0f},
    {"Water",     "Rare",       10,        0.0f, 0.5f, 1.0f},
    {"Shadow",    "Epic",        5,        0.6f, 0.1f, 0.9f},
    {"Space",     "Legendary",   1.999999f,1.0f, 0.5f, 0.0f},
    {"Infinity",  "Mythical",    0.000001f,1.0f, 0.84f,0.0f}
};


static void SpawnBurst(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (rand() % 360) * (3.14159f / 180.0f);
        float speed = (rand() % 100) / 1000.0f + 0.01f;
        s_particles.push_back({ x, y, cosf(angle) * speed, sinf(angle) * speed, 1.0f, r, g, b });
    }
}

WordEntry RollGachaWord() {
    float totalWeight = 0.0f;
    for (const auto& entry : gachaPool) totalWeight += entry.weight;

    float roll = (float)rand() / (float)RAND_MAX * totalWeight;
    for (const auto& entry : gachaPool) {
        if (roll < entry.weight) return entry;
        roll -= entry.weight;
    }
    return gachaPool.back();
}

static int RarityRank(const std::string& r) {
    if (r == "Common")    return 0;
    if (r == "Uncommon")  return 1;
    if (r == "Rare")      return 2;
    if (r == "Epic")      return 3;
    if (r == "Legendary") return 4;
    if (r == "Mythical")  return 5;
    return -1;
}

static WordEntry FindHighestRarity(const std::vector<WordEntry>& results) {
    if (results.empty())
        return { "", "Common", 1.0f, 1.0f, 1.0f, 1.0f };

    WordEntry best = results[0];
    for (const auto& e : results) {
        if (RarityRank(e.rarity) > RarityRank(best.rarity))
            best = e;
    }
    return best;
}

static void SetIdentityTransform() {
    AEMtx33 S, T, I;
    AEMtx33Scale(&S, 1.0f, 1.0f);
    AEMtx33Trans(&T, 0.0f, 0.0f);
    AEMtx33Concat(&I, &T, &S);
    AEGfxSetTransform(I.m);
}

static void DrawSolidRect(const AEMtx33& parent,
    float x, float y, float w, float h,
    float mulR, float mulG, float mulB, float mulA,
    float addR, float addG, float addB, float addA,
    AEGfxBlendMode bm)
{
    if (!s_overlayQuad) return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(bm);

    AEGfxSetColorToMultiply(mulR, mulG, mulB, mulA);
    AEGfxSetColorToAdd(addR, addG, addB, addA);
    AEMtx33 S, T, local, M;
    AEMtx33Scale(&S, w, h);
    AEMtx33Trans(&T, x, y);
    AEMtx33Concat(&local, &T, &S);
    AEMtx33Concat(&M, &parent, &local);
    AEGfxSetTransform(M.m);
    AEGfxMeshDraw(s_overlayQuad, AE_GFX_MDM_TRIANGLES);
}

void EnsureOverlayMesh() {
    if (s_overlayQuad) return;

    AEGfxMeshStart();

    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 1.0f
    );
    AEGfxTriAdd(
        -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 1.0f
    );

    s_overlayQuad = AEGfxMeshEnd();
}

void UpdateGachaAnimation(GachaAnimation& anim, float dt) {
    if (s_screenShake > 0) s_screenShake -= dt * 2.0f;

    for (int i = 0; i < (int)s_particles.size();) {
        s_particles[i].x += s_particles[i].vx;
        s_particles[i].y += s_particles[i].vy;
        s_particles[i].life -= dt * 2.0f;

        if (s_particles[i].life <= 0) {
            s_particles[i] = s_particles.back();
            s_particles.pop_back();
        }
        else {
            ++i;
        }
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

    // Reset chest/shines
    s_chestOpened = false;
    s_chestLightTimer = 0.0f;

    for (int i = 0; i < rollCount; ++i) {
        WordEntry res = RollGachaWord();
        anim.results.push_back(res);
        s_rarityCounts[res.rarity]++;
    }

    // Highest rarity color
    s_highestEntry = FindHighestRarity(anim.results);

    anim.phase = GachaPhase::Intro;
    anim.introTimer = introTime;

    anim.rollingTimer = rollingTime;

    anim.revealDelay = revealDelay;
}

void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skipPressed, bool openPressed) {
    if (anim.phase == GachaPhase::None) return;
    anim.elapsed += dt;

    // skip to results
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
        if (anim.introTimer <= 0.0f)
            anim.phase = GachaPhase::Rolling; 
        break;

    case GachaPhase::Rolling:
        if (!s_chestOpened) {
            if (openPressed) {
                s_chestOpened = true;
                s_chestLightTimer = s_chestLightMax;
                // Burst + shake on open (uses highest rarity color)
                SpawnBurst(0.0f, 0.0f, s_highestEntry.r, s_highestEntry.g, s_highestEntry.b, 260);
                s_screenShake = 0.45f;
            }
        }
        else {
            s_chestLightTimer -= dt;
            if (s_chestLightTimer <= 0.0f) {
                s_chestLightTimer = 0.0f;
                anim.phase = GachaPhase::Reveal;
                anim.timer = 0.0f;
            }
        }
        break;

    case GachaPhase::Reveal:
    case GachaPhase::Done:
        UpdateGachaAnimation(anim, dt);
        break;

    default:
        break;
    }
}


void DrawGachaOverlay(GachaAnimation& anim, s8 fontId) {

    AEGfxSetCamPosition(0.0f, 0.0f);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);

    float oX = (s_screenShake > 0) ? ((rand() % 100) / 1000.0f - 0.05f) * s_screenShake * 10.0f : 0.0f;
    float oY = (s_screenShake > 0) ? ((rand() % 100) / 1000.0f - 0.05f) * s_screenShake * 10.0f : 0.0f;
    AEMtx33 shake; AEMtx33Trans(&shake, oX, oY);
    AEGfxSetTransform(shake.m);

    // Particles
    for (auto& p : s_particles)
        AEGfxPrint(fontId, ".", p.x, p.y, 1.2f, p.r, p.g, p.b, p.life);

    if (anim.phase == GachaPhase::Rolling) {

        float minX = AEGfxGetWinMinX();
        float maxX = AEGfxGetWinMaxX();
        float minY = AEGfxGetWinMinY();
        float maxY = AEGfxGetWinMaxY();
        float winW = (maxX - minX);
        float winH = (maxY - minY);

        // chest
        float boxY = -winH * 0.08f;
        DrawSolidRect(
            shake,
            0.0f, 0.0f,
            winW * 0.30f, winH * 0.22f,
            1.0f, 1.0f, 0.0f, 1.0f,    
            0.0f, 0.0f, 0.0f, 0.0f,     
            AE_GFX_BM_BLEND
        );

        // Reveal highest rarerity light
        if (s_chestOpened && s_chestLightTimer > 0.0f)
        {
            float t = s_chestLightTimer / s_chestLightMax; 
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            float pulse = 0.5f + 0.5f * sinf((1.0f - t) * 12.0f); 
            float alpha = (0.25f + 0.55f * pulse) * t;          
            AEMtx33 ident;
            AEMtx33Trans(&ident, 0.0f, 0.0f);
            DrawSolidRect(
                ident,
                0.0f, 0.0f,
                winW * 1.05f, winH * 1.05f,
                s_highestEntry.r, s_highestEntry.g, s_highestEntry.b, alpha, 
                0.0f, 0.0f, 0.0f, 0.0f,                                      
                AE_GFX_BM_BLEND                                              
            );
        }
       
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1, 1, 1, 1);
        AEGfxSetColorToAdd(0, 0, 0, 0);
        SetIdentityTransform();

        if (!s_chestOpened) {
            AEGfxPrint(fontId, "[O] Open", -0.12f, -0.9f, 0.8f, 1, 1, 1, 1);
        }

        return; 
    }

    if (anim.results.size() <= 10) {
        // 10-PULL GRID (3-3-3-1)
        for (int i = 0; i <= anim.currentIndex; ++i) {
            const auto& e = anim.results[i];
            float x = 0.0f, y = 0.0f;
            float sc = 1.3f;

            if (i < 3) { x = -0.55f + (i * 0.55f); y = 0.70f; }
            else if (i < 6) { x = -0.55f + ((i - 3) * 0.55f); y = 0.40f; }
            else if (i < 9) { x = -0.55f + ((i - 6) * 0.55f); y = 0.10f; }
            else { x = 0.0f; y = -0.20f; }

            float offset = -((float)e.word.length() * sc * 0.022f);
            AEGfxPrint(fontId, e.word.c_str(), x + offset, y, sc, e.r, e.g, e.b, 1.0f);
        }
    }
    else {
        // 100-PULL COMPACT GRID
        for (int i = 0; i <= anim.currentIndex; ++i) {
            int row = i / 10;
            int col = i % 10;
            const auto& e = anim.results[i];

            if (i == anim.currentIndex &&
                (e.rarity == "Legendary" || e.rarity == "Mythical") &&
                !anim.isFinished)
            {
                AEGfxPrint(fontId, "!!! NEW !!!", -0.12f, 0.25f, 1.0f, 1, 1, 0, 1);
                AEGfxPrint(fontId, e.word.c_str(), -0.25f, 0.0f, 4.0f, e.r, e.g, e.b, 1.0f);
                AEGfxPrint(fontId, e.rarity.c_str(), -0.15f, -0.2f, 1.2f, e.r, e.g, e.b, 1.0f);
            }
            else {
                AEGfxPrint(fontId, ".", -0.72f + (col * 0.16f), 0.65f - (row * 0.11f),
                    1.2f, e.r, e.g, e.b, 1.0f);
            }
        }

        if (anim.phase == GachaPhase::Done) {
            std::string s = "MYTHIC: " + std::to_string(s_rarityCounts["Mythical"]) +
                " | LEGEND: " + std::to_string(s_rarityCounts["Legendary"]);
            AEGfxPrint(fontId, s.c_str(), -0.42f, -0.65f, 0.8f, 1, 1, 0, 1);
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
