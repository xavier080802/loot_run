#include "Gacha.h"
#include "AEEngine.h"
#include "Pets/PetManager.h"
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <map>

static float s_screenShake = 0.0f;
static bool  s_chestOpened = false;
static float s_chestLightTimer = 0.0f;
static const float s_chestLightMax = 0.85f;
static float s_chestBounceTimer = 0.0f;
static float s_chestBounceOffset = 0.0f;
static float s_idleGlowTimer = 0.0f;
static WordEntry s_highestEntry;
static AEGfxVertexList* s_overlayQuad = nullptr;

struct GachaParticle {
    float x, y, vx, vy, life, r, g, b;
    float floorY;
};
static std::vector<GachaParticle> s_particles;

static void SpawnBurst(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; ++i) {
        float a = (rand() % 360) * 0.0174533f;
        float s = (rand() % 100) / 600.0f + 0.02f;
        float vx = cosf(a) * s;
        float vy = sinf(a) * s + 0.02f;
        float pFloor = -0.85f - ((rand() % 100) / 1000.0f);
        s_particles.push_back({ x, y, vx, vy, 1.2f, r, g, b, pFloor });
    }
}

static std::vector<WordEntry> gachaPool = {
    {"Rock",      "Common",     63,        1.0f, 1.0f, 1.0f},
    {"Slime",    "Uncommon",   20,        0.0f, 1.0f, 0.0f},
    {"Wolf",     "Rare",       10,        0.0f, 0.5f, 1.0f},
    {"Whale",    "Epic",        5,        0.6f, 0.1f, 0.9f},
    {"Garuda",     "Legendary",   1.999999f,1.0f, 0.5f, 0.0f},
    {"Dragon",  "Mythical",    0.000001f,1.0f, 0.84f,0.0f}
};

int RarityRank(const std::string& r) {
    if (r == "Common") return 0;
    if (r == "Uncommon") return 1;
    if (r == "Rare") return 2;
    if (r == "Epic") return 3;
    if (r == "Legendary") return 4;
    if (r == "Mythical") return 5;
    return -1;
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

static WordEntry FindHighestRarity(const std::vector<WordEntry>& r) {
    WordEntry best = r[0];
    for (auto& e : r)
        if (RarityRank(e.rarity) > RarityRank(best.rarity))
            best = e;
    return best;
}

void EnsureOverlayMesh() {
    if (s_overlayQuad) return;
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, -0.5f, 0xFFFFFFFF, 1, 0, 0.5f, 0.5f, 0xFFFFFFFF, 1, 1);
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, 0.5f, 0xFFFFFFFF, 1, 1, -0.5f, 0.5f, 0xFFFFFFFF, 0, 1);
    s_overlayQuad = AEGfxMeshEnd();
}

static void DrawSolidRect(const AEMtx33& parent, float x, float y, float w, float h, float r, float g, float b, float a) {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxTextureSet(NULL, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(r, g, b, a);
    AEMtx33 S, T, M;
    AEMtx33Scale(&S, w, h);
    AEMtx33Trans(&T, x, y);
    AEMtx33Concat(&M, &T, &S);
    AEMtx33Concat(&M, &parent, &M);
    AEGfxSetTransform(M.m);
    if (s_overlayQuad) AEGfxMeshDraw(s_overlayQuad, AE_GFX_MDM_TRIANGLES);
}

void BeginGachaOverlay(GachaAnimation& anim, int count, float intro, float roll, float delay) {
    anim.Reset();
    anim.currentIndex = -1;
    s_particles.clear();
    s_chestOpened = false;
    //Roll
    for (int i = 0; i < count; ++i) anim.results.push_back(RollGachaWord());
    s_highestEntry = FindHighestRarity(anim.results);
    anim.phase = GachaPhase::Intro;
    anim.introTimer = intro;
    anim.revealDelay = delay;
}

void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skip, bool open) {
    if (anim.phase == GachaPhase::Intro) {
        anim.introTimer -= dt;
        if (anim.introTimer <= 0.0f) anim.phase = GachaPhase::Rolling;
        return;
    }
    s_idleGlowTimer += dt;
    if (s_chestBounceTimer > 0.0f) {
        s_chestBounceTimer -= dt;
        s_chestBounceOffset = sinf((s_chestBounceTimer / 0.35f) * 3.14159f) * 15.0f;
    }
    else s_chestBounceOffset = 0.0f;

    //Check if chest is opened
    if (anim.phase == GachaPhase::Rolling && open && !s_chestOpened) {
        s_chestOpened = true;
        s_chestBounceTimer = 0.35f;
        s_chestLightTimer = s_chestLightMax;
        SpawnBurst(0, 0, s_highestEntry.r, s_highestEntry.g, s_highestEntry.b, 260);

        //Save rolls to db
        for (WordEntry const& e : anim.results) {
            //TODO: get actual pet id and rarity
            PetManager::GetInstance()->AddNewPet(Pets::PetSaveData{ Pets::PET_1, Pets::LEGENDARY });
        }
    }

    if (s_chestOpened && anim.phase == GachaPhase::Rolling) {
        s_chestLightTimer -= dt;
        if (s_chestLightTimer <= 0.0f) { anim.phase = GachaPhase::Reveal; anim.timer = 0.0f; }
    }

    if (anim.phase == GachaPhase::Reveal) {
        if (skip) {
            for (int i = anim.currentIndex + 1; i < (int)anim.results.size(); ++i) {
                if (RarityRank(anim.results[i].rarity) >= 4) {
                    anim.currentIndex = i - 1; anim.timer = 0.0f; goto skip_stop;
                }
                anim.currentIndex = i;
            }
            anim.phase = GachaPhase::Done; anim.isFinished = true; return;
        }
    skip_stop:
        if (open) anim.timer = -1.0f;
        anim.timer -= dt;
        if (anim.timer <= 0.0f) {
            anim.currentIndex++;
            if (anim.currentIndex < (int)anim.results.size()) {
                auto& e = anim.results[anim.currentIndex];
                float bX = 0, bY = 0;
                int i = anim.currentIndex;
                if (anim.results.size() <= 10) {
                    if (i < 3) { bX = -0.55f + i * 0.55f; bY = 0.70f; }
                    else if (i < 6) { bX = -0.55f + (i - 3) * 0.55f; bY = 0.40f; }
                    else if (i < 9) { bX = -0.55f + (i - 6) * 0.55f; bY = 0.10f; }
                    else { bX = 0.0f; bY = -0.20f; }
                }
                else {
                    bX = -0.72f + ((i % 10) * 0.16f); bY = 0.65f - ((i / 10) * 0.11f);
                }
                SpawnBurst(bX, bY, e.r, e.g, e.b, (RarityRank(e.rarity) >= 4 ? 200 : 25));
                anim.timer = (RarityRank(e.rarity) >= 4) ? 1.5f : anim.revealDelay;
            }
            else { anim.phase = GachaPhase::Done; anim.isFinished = true; }
        }
    }

    for (int i = 0; i < (int)s_particles.size();) {
        s_particles[i].vy -= dt * 0.35f;
        s_particles[i].x += s_particles[i].vx; s_particles[i].y += s_particles[i].vy;
        if (s_particles[i].y < s_particles[i].floorY) { s_particles[i].y = s_particles[i].floorY; s_particles[i].vy *= -0.5f; }
        s_particles[i].life -= dt;
        if (s_particles[i].life <= 0) { s_particles[i] = s_particles.back(); s_particles.pop_back(); }
        else ++i;
    }
}

void DrawGachaOverlay(GachaAnimation& anim, s8 fontId) {
    EnsureOverlayMesh();
    AEMtx33 I; AEMtx33Identity(&I);
    for (auto& p : s_particles) { AEGfxPrint(fontId, ".", p.x, p.y, 1.2f, p.r, p.g, p.b, p.life); }

    if (anim.phase == GachaPhase::Reveal || anim.phase == GachaPhase::Done) {
        for (int i = 0; i <= anim.currentIndex; ++i) {
            if (i >= (int)anim.results.size()) continue;
            auto& e = anim.results[i];

            if (i == anim.currentIndex && RarityRank(e.rarity) >= 4 && !anim.isFinished) {
                AEGfxPrint(fontId, "!!! NEW !!!", -0.16f, 0.30f, 2.0f, 1, 1, 0, 1);
                float charOffset = (e.word.length() * 0.045f);
                AEGfxPrint(fontId, e.word.c_str(), -charOffset, 0.05f, 4.0f, e.r, e.g, e.b, 1.0f);
            }
            else {
                float x = 0, y = 0;
                if (anim.results.size() <= 10) {
                    if (i < 3) { x = -0.55f + i * 0.55f; y = 0.70f; }
                    else if (i < 6) { x = -0.55f + (i - 3) * 0.55f; y = 0.40f; }
                    else if (i < 9) { x = -0.55f + (i - 6) * 0.55f; y = 0.10f; }
                    else { x = 0.0f; y = -0.20f; }
                    AEGfxPrint(fontId, e.word.c_str(), x - (e.word.length() * 0.02f), y, 1.3f, e.r, e.g, e.b, 1.0f);
                }
                else {
                    x = -0.72f + ((i % 10) * 0.16f); y = 0.65f - ((i / 10) * 0.11f);
                    AEGfxPrint(fontId, ".", x, y, 1.2f, e.r, e.g, e.b, 1.0f);
                }
            }
        }
        if (anim.phase == GachaPhase::Done) { AEGfxPrint(fontId, "[SPACE] Skip | [R] 10x | [T] 100x | [ESC] Exit", -0.50f, -0.9f, 0.85f, 1, 1, 1, 1); }
        else { AEGfxPrint(fontId, "[SPACE] Fast-Forward | [ESC] Exit", -0.25f, -0.9f, 0.85f, 1, 1, 1, 1); }
    }

    if (anim.phase == GachaPhase::Rolling) {
        float glow = 0.25f + 0.15f * sinf(s_idleGlowTimer * 2.0f);
        DrawSolidRect(I, 0.0f, -110.0f + s_chestBounceOffset, 400.0f, 300.0f, 1.0f, 1.0f, 0.0f, s_chestOpened ? 1.0f : glow);
        if (!s_chestOpened) {
            AEGfxPrint(fontId, "[O]: Open Chest          [ESC]: Quit", -0.20f, -0.75f, 0.85f, 1, 1, 1, 1);
        }
    }
}

void UnloadGacha()
{
    AEGfxMeshFree(s_overlayQuad);
}
