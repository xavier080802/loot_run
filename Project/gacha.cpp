#include "Gacha.h"
#include "AEEngine.h"
#include "Pets/PetManager.h"
#include "Pets/PetInventory.h"
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

static float s_mythicalFlashTimer = 0.0f;
static float s_mythicalPulseTimer = 0.0f;
static bool  s_isMythicalRevealing = false;

static float s_legendaryPulseTimer = 0.0f;

static bool s_showRates = false;

struct GachaParticle {
    float x, y;
    float vx, vy;
    float life;
    float r, g, b;
    float floorY;
};
static std::vector<GachaParticle> s_particles;

static Pets::PET_TYPE GetPetTypeFromWord(const std::string& word) {
    auto const& petmap = PetManager::GetInstance()->GetPetDataMap();
    for (auto it{ petmap.begin() }; it != petmap.end(); ++it) {
        if (it->second.name == word) return it->first;
    }
    return Pets::PET_TYPE::NONE;
}

static void SpawnBurst(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (rand() % 360) * 0.0174533f;
        float speed = (rand() % 100) / 500.0f + 0.015f;
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed + 0.015f;
        float pFloor = -0.85f - ((rand() % 100) / 800.0f);
        float life = 1.0f + (rand() % 100) / 80.0f;
        float bright = 0.75f + (rand() % 25) / 100.0f;
        s_particles.push_back({ x, y, vx, vy, life, r * bright, g * bright, b * bright, pFloor });
    }
}

static void SpawnEpicBurst(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (rand() % 360) * 0.0174533f;
        float speed = (rand() % 100) / 250.0f + 0.03f;
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed + 0.03f;
        float pFloor = -0.85f - ((rand() % 100) / 600.0f);
        float life = 1.5f + (rand() % 100) / 60.0f;
        float t = (rand() % 100) / 100.0f;
        float pr = r + (1.0f - r) * t * 0.4f;
        float pg = g + (1.0f - g) * t * 0.4f;
        float pb = b + (1.0f - b) * t * 0.4f;
        s_particles.push_back({ x, y, vx, vy, life, pr, pg, pb, pFloor });
    }
}

static void SpawnMythicalBurst(float x, float y, int count) {
    static const float palette[3][3] = {
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.4f, 0.0f },
        { 0.8f, 0.0f, 0.0f },
    };
    for (int i = 0; i < count; ++i) {
        float angle = (rand() % 360) * 0.0174533f;
        float speed = (rand() % 100) / 300.0f + 0.04f;
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed + 0.04f;
        float pFloor = -0.85f - ((rand() % 100) / 500.0f);
        float life = 1.8f + (rand() % 100) / 100.0f;
        int col = rand() % 3;
        s_particles.push_back({ x, y, vx, vy, life, palette[col][0], palette[col][1], palette[col][2], pFloor });
    }
}

static std::vector<WordEntry> gachaPool = {
    {"Rock",    "Common",    15.799995f, 1.0f, 1.0f, 1.0f},
    {"Slime",   "Common",    15.75f,     1.0f, 1.0f, 1.0f},
    {"Lycan",   "Common",    15.75f,     1.0f, 1.0f, 1.0f},
    {"Scylla",  "Common",    15.75f,     1.0f, 1.0f, 1.0f},
    {"Rock",    "Uncommon",  5.0f,       0.0f, 1.0f, 0.0f},
    {"Slime",   "Uncommon",  5.0f,       0.0f, 1.0f, 0.0f},
    {"Lycan",   "Uncommon",  5.0f,       0.0f, 1.0f, 0.0f},
    {"Scylla",  "Uncommon",  5.0f,       0.0f, 1.0f, 0.0f},
    {"Rock",    "Rare",      2.5f,       0.0f, 0.5f, 1.0f},
    {"Slime",   "Rare",      2.5f,       0.0f, 0.5f, 1.0f},
    {"Lycan",   "Rare",      2.5f,       0.0f, 0.5f, 1.0f},
    {"Scylla",  "Rare",      2.5f,       0.0f, 0.5f, 1.0f},
    {"Rock",    "Epic",      1.25f,      0.6f, 0.1f, 0.9f},
    {"Slime",   "Epic",      1.25f,      0.6f, 0.1f, 0.9f},
    {"Lycan",   "Epic",      1.25f,      0.6f, 0.1f, 0.9f},
    {"Scylla",  "Epic",      1.25f,      0.6f, 0.1f, 0.9f},
    {"Rock",    "Legendary", 0.19f,      1.0f, 0.5f, 0.0f},
    {"Slime",   "Legendary", 0.19f,      1.0f, 0.5f, 0.0f},
    {"Lycan",   "Legendary", 0.19f,      1.0f, 0.5f, 0.0f},
    {"Scylla",  "Legendary", 0.19f,      1.0f, 0.5f, 0.0f},
    {"Phoenix", "Legendary", 0.19f,      1.0f, 0.5f, 0.0f},
    {"Dragon",  "Mythical",  0.000005f,  1.0f, 0.0f, 0.0f},
};

int RarityRank(const std::string& r) {
    if (r == "Common")    return 0;
    if (r == "Uncommon")  return 1;
    if (r == "Rare")      return 2;
    if (r == "Epic")      return 3;
    if (r == "Legendary") return 4;
    if (r == "Mythical")  return 5;
    return -1;
}

static WordEntry RollGachaWord() {
    float total = 0.0f;
    for (auto& e : gachaPool) total += e.weight;
    float roll = ((float)rand() / RAND_MAX) * total;
    for (auto& e : gachaPool) {
        if (roll < e.weight) return e;
        roll -= e.weight;
    }
    return gachaPool.back();
}

static WordEntry FindHighestRarity(const std::vector<WordEntry>& results) {
    WordEntry best = results[0];
    for (auto& e : results)
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

static bool IsMouseOverChest() {
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    float winW = (float)(AEGfxGetWinMaxX() * 2);
    float winH = (float)(AEGfxGetWinMaxY() * 2);

    float worldX = ((float)mx / (winW * 0.5f)) - 1.0f;
    float worldY = 1.0f - ((float)my / (winH * 0.5f));

    float halfW = 400.0f / winW;
    float halfH = 300.0f / winH;
    float chestY = s_chestBounceOffset / (winH * 0.5f);

    return (worldX >= -halfW && worldX <= halfW &&
        worldY >= chestY - halfH && worldY <= chestY + halfH);
}

static void DrawRatesPanel(s8 fontId) {
    EnsureOverlayMesh();

    // Full screen black overlay drawn directly - no DrawSolidRect
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxTextureSet(NULL, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.92f);
    AEMtx33 fullScreen;
    AEMtx33Scale(&fullScreen, 2.0f, 2.0f);
    AEGfxSetTransform(fullScreen.m);
    AEGfxMeshDraw(s_overlayQuad, AE_GFX_MDM_TRIANGLES);

    // Panel background
    AEGfxSetColorToMultiply(0.08f, 0.08f, 0.12f, 1.0f);
    AEMtx33 panelMtx;
    AEMtx33Scale(&panelMtx, 0.95f, 1.55f);
    AEGfxSetTransform(panelMtx.m);
    AEGfxMeshDraw(s_overlayQuad, AE_GFX_MDM_TRIANGLES);

    AEMtx33 I; AEMtx33Identity(&I);

    // Panel border
    DrawSolidRect(I, 0.0f, 0.785f, 0.95f, 0.015f, 0.4f, 0.4f, 0.6f, 1.0f);
    DrawSolidRect(I, 0.0f, -0.785f, 0.95f, 0.015f, 0.4f, 0.4f, 0.6f, 1.0f);
    DrawSolidRect(I, -0.485f, 0.0f, 0.015f, 1.57f, 0.4f, 0.4f, 0.6f, 1.0f);
    DrawSolidRect(I, 0.485f, 0.0f, 0.015f, 1.57f, 0.4f, 0.4f, 0.6f, 1.0f);

    struct RarityInfo { const char* name; float r, g, b; };
    static const RarityInfo tiers[] = {
        { "Mythical",  1.0f, 0.0f, 0.0f },
        { "Legendary", 1.0f, 0.5f, 0.0f },
        { "Epic",      0.6f, 0.1f, 0.9f },
        { "Rare",      0.0f, 0.5f, 1.0f },
        { "Uncommon",  0.0f, 1.0f, 0.0f },
        { "Common",    1.0f, 1.0f, 1.0f },
    };

    std::map<std::string, float> totalWeights;
    float grandTotal = 0.0f;
    for (auto& e : gachaPool) { totalWeights[e.rarity] += e.weight; grandTotal += e.weight; }

    float titleScale = 2.2f;
    float titleHalfW = (5.0f * 0.0135f * titleScale) / 2.0f;
    AEGfxPrint(fontId, "RATES", -titleHalfW, 0.62f, titleScale, 1.0f, 0.85f, 0.2f, 1.0f);

    DrawSolidRect(I, 0.0f, 0.54f, 0.8f, 0.006f, 0.6f, 0.5f, 0.1f, 1.0f);

    float startY = 0.42f;
    float stepY = 0.165f;
    float rowScale = 1.55f;

    for (int i = 0; i < 6; ++i) {
        auto& tier = tiers[i];
        float pct = (totalWeights[tier.name] / grandTotal) * 100.0f;

        int petCount = 0;
        for (auto& e : gachaPool) if (e.rarity == tier.name) petCount++;

        float y = startY - i * stepY;

        if (i % 2 == 0)
            DrawSolidRect(I, 0.0f, y, 0.88f, stepY * 0.85f, 1.0f, 1.0f, 1.0f, 0.04f);

        char nameBuf[32];
        sprintf_s(nameBuf, "%s", tier.name);
        AEGfxPrint(fontId, nameBuf, -0.42f, y, rowScale, tier.r, tier.g, tier.b, 1.0f);

        char pctBuf[32];
        sprintf_s(pctBuf, "%.6f%%", pct);
        float pctW = ((float)strlen(pctBuf) * 0.0135f * rowScale);
        AEGfxPrint(fontId, pctBuf, -pctW / 2.0f, y, rowScale, 1.0f, 1.0f, 1.0f, 1.0f);

        char petBuf[32];
        sprintf_s(petBuf, "%d pets", petCount);
        float petW = ((float)strlen(petBuf) * 0.0135f * rowScale);
        AEGfxPrint(fontId, petBuf, 0.42f - petW, y, rowScale, 0.6f, 0.6f, 0.6f, 1.0f);
    }

    const char* hint = "[I] Close";
    float hintScale = 1.3f;
    float hintW = ((float)strlen(hint) * 0.0135f * hintScale);
    AEGfxPrint(fontId, hint, -hintW / 2.0f, -0.70f, hintScale, 0.5f, 0.5f, 0.5f, 1.0f);
}

void BeginGachaOverlay(GachaAnimation& anim, int count, float intro, float /*roll*/, float delay) {
    anim.Reset();
    anim.currentIndex = -1;
    s_particles.clear();
    s_chestOpened = false;
    s_mythicalFlashTimer = 0.0f;
    s_mythicalPulseTimer = 0.0f;
    s_legendaryPulseTimer = 0.0f;
    s_isMythicalRevealing = false;
    s_showRates = false;
    for (int i = 0; i < count; ++i) anim.results.push_back(RollGachaWord());
    s_highestEntry = FindHighestRarity(anim.results);
    anim.phase = GACHA_PHASE::INTRO;
    anim.introTimer = intro;
    anim.revealDelay = delay;
}

void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skip, bool open) {
    if (anim.phase == GACHA_PHASE::INTRO) {
        anim.introTimer -= dt;
        if (anim.introTimer <= 0.0f) anim.phase = GACHA_PHASE::ROLLING;
        return;
    }

    s_idleGlowTimer += dt;
    s_mythicalPulseTimer += dt;
    s_legendaryPulseTimer += dt;

    if (s_chestBounceTimer > 0.0f) {
        s_chestBounceTimer -= dt;
        s_chestBounceOffset = sinf((s_chestBounceTimer / 0.35f) * 3.14159f) * 15.0f;
    }
    else s_chestBounceOffset = 0.0f;

    if (anim.phase == GACHA_PHASE::ROLLING) {
        if (AEInputCheckTriggered(AEVK_I)) s_showRates = !s_showRates;

        if (!s_showRates) {
            bool mouseOpen = AEInputCheckTriggered(AEVK_LBUTTON) && IsMouseOverChest();
            bool keyOpen = open;

            if ((mouseOpen || keyOpen) && !s_chestOpened) {
                s_chestOpened = true;
                s_chestBounceTimer = 0.35f;
                s_chestLightTimer = s_chestLightMax;
                if (RarityRank(s_highestEntry.rarity) >= 5) {
                    SpawnMythicalBurst(0.0f, 0.0f, 600);
                    SpawnMythicalBurst(-0.3f, 0.1f, 200);
                    SpawnMythicalBurst(0.3f, 0.1f, 200);
                    s_mythicalFlashTimer = 0.6f;
                    s_isMythicalRevealing = true;
                }
                else {
                    SpawnBurst(0.0f, 0.0f, s_highestEntry.r, s_highestEntry.g, s_highestEntry.b, 260);
                }
                for (WordEntry const& e : anim.results) {
                    PetManager::GetInstance()->AddNewPet(Pets::PetSaveData{ GetPetTypeFromWord(e.word), static_cast<Pets::PET_RANK>(RarityRank(e.rarity)) });
                }
                PetManager::GetInstance()->SaveInventoryToJSON();
            }
        }
    }

    if (s_chestOpened && anim.phase == GACHA_PHASE::ROLLING) {
        s_chestLightTimer -= dt;
        if (s_chestLightTimer <= 0.0f) {
            anim.phase = GACHA_PHASE::REVEAL;
            anim.timer = 0.0f;
        }
    }

    if (s_mythicalFlashTimer > 0.0f) s_mythicalFlashTimer -= dt;

    if (s_isMythicalRevealing && anim.phase == GACHA_PHASE::REVEAL) {
        int idx = anim.currentIndex;
        if (idx >= 0 && idx < (int)anim.results.size()) {
            if (RarityRank(anim.results[idx].rarity) >= 5) {
                if (rand() % 3 == 0) SpawnMythicalBurst(0.0f, 0.05f, 15);
            }
            else s_isMythicalRevealing = false;
        }
    }

    if (anim.phase == GACHA_PHASE::REVEAL) {
        if (skip) {
            for (int i = anim.currentIndex + 1; i < (int)anim.results.size(); ++i) {
                if (RarityRank(anim.results[i].rarity) >= 4) {
                    anim.currentIndex = i - 1;
                    anim.timer = 0.0f;
                    goto skip_stop;
                }
                anim.currentIndex = i;
            }
            anim.phase = GACHA_PHASE::DONE;
            anim.isFinished = true;
            s_isMythicalRevealing = false;
            return;
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
                    if (i < 3) { bX = -0.55f + i * 0.55f;       bY = 0.70f; }
                    else if (i < 6) { bX = -0.55f + (i - 3) * 0.55f; bY = 0.40f; }
                    else if (i < 9) { bX = -0.55f + (i - 6) * 0.55f; bY = 0.10f; }
                    else { bX = 0.0f;                       bY = -0.20f; }
                }
                else {
                    bX = -0.72f + ((i % 10) * 0.16f);
                    bY = 0.65f - ((i / static_cast<float>(10)) * 0.11f);
                }
                if (RarityRank(e.rarity) >= 5) {
                    SpawnMythicalBurst(bX, bY, 500);
                    SpawnMythicalBurst(bX - 0.2f, bY + 0.1f, 150);
                    SpawnMythicalBurst(bX + 0.2f, bY + 0.1f, 150);
                    s_isMythicalRevealing = true; s_mythicalFlashTimer = 0.4f; anim.timer = 2.5f;
                }
                else if (RarityRank(e.rarity) == 4) {
                    SpawnEpicBurst(bX, bY, e.r, e.g, e.b, 180);
                    SpawnEpicBurst(bX - 0.15f, bY + 0.05f, e.r, e.g, e.b, 80);
                    SpawnEpicBurst(bX + 0.15f, bY + 0.05f, e.r, e.g, e.b, 80);
                    anim.timer = 2.0f;
                }
                else if (RarityRank(e.rarity) == 3) {
                    SpawnEpicBurst(bX, bY, e.r, e.g, e.b, 120); anim.timer = 1.2f;
                }
                else {
                    SpawnBurst(bX, bY, e.r, e.g, e.b, 40); anim.timer = anim.revealDelay;
                }
            }
            else {
                anim.phase = GACHA_PHASE::DONE; anim.isFinished = true; s_isMythicalRevealing = false;
            }
        }
    }

    for (int i = 0; i < (int)s_particles.size();) {
        auto& p = s_particles[i];
        p.vy -= dt * 0.4f; p.vx *= (1.0f - dt * 0.8f); p.vy *= (1.0f - dt * 0.3f);
        p.x += p.vx; p.y += p.vy;
        if (p.y < p.floorY) { p.y = p.floorY; p.vy *= -0.35f; p.vx *= 0.7f; }
        p.life -= dt;
        if (p.life <= 0) { s_particles[i] = s_particles.back(); s_particles.pop_back(); }
        else ++i;
    }
}

void DrawGachaOverlay(GachaAnimation& anim, s8 fontId) {
    EnsureOverlayMesh();
    AEMtx33 I; AEMtx33Identity(&I);

    // Rates panel is drawn first and returns early so nothing bleeds through
    if (s_showRates) {
        DrawRatesPanel(fontId);
        return;
    }

    if (s_mythicalFlashTimer > 0.0f) {
        float alpha = (s_mythicalFlashTimer / 0.6f) * 0.55f;
        DrawSolidRect(I, 0.0f, 0.0f, 2.0f, 2.0f, 1.0f, 0.0f, 0.0f, alpha);
    }

    for (auto& p : s_particles) {
        float fadeAlpha = (p.life > 1.0f) ? 1.0f : p.life;
        float sz = 0.8f + p.life * 0.3f;
        AEGfxPrint(fontId, (p.life > 1.2f) ? "*" : ".", p.x, p.y, sz, p.r, p.g, p.b, fadeAlpha);
    }

    if (anim.phase == GACHA_PHASE::REVEAL || anim.phase == GACHA_PHASE::DONE) {
        for (int i = 0; i <= anim.currentIndex; ++i) {
            if (i >= (int)anim.results.size()) continue;
            auto& e = anim.results[i];
            bool isMythical = (RarityRank(e.rarity) >= 5);
            bool isLegendary = (RarityRank(e.rarity) >= 4);

            if (i == anim.currentIndex && isLegendary && !anim.isFinished) {
                if (isMythical) {
                    float pulse = 5.5f + 0.6f * sinf(s_mythicalPulseTimer * 6.0f);
                    float labelAlpha = 0.6f + 0.4f * sinf(s_mythicalPulseTimer * 8.0f);
                    float glowAlpha = 0.25f + 0.15f * sinf(s_mythicalPulseTimer * 5.0f);
                    float cx = -((float)e.word.length() * 0.0135f * pulse) / 2.0f;

                    AEGfxPrint(fontId, e.word.c_str(), cx - 0.03f, 0.05f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx + 0.03f, 0.05f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx, 0.08f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx, 0.02f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx, 0.05f, pulse, 1.0f, 0.0f, 0.0f, 1.0f);

                    float newLabelScale = 2.5f;
                    float newLabelHalfW = (11.0f * 0.0135f * newLabelScale) / 2.0f;
                    AEGfxPrint(fontId, "!!! NEW !!!", -newLabelHalfW, 0.30f, newLabelScale, 1.0f, 0.2f, 0.0f, labelAlpha);
                }
                else {
                    float legPulse = 5.0f + 0.3f * sinf(s_legendaryPulseTimer * 5.0f);
                    float legAlpha = 0.7f + 0.3f * sinf(s_legendaryPulseTimer * 5.0f);
                    float cx = -((float)e.word.length() * 0.0135f * legPulse) / 2.0f;

                    float newLabelScale = 2.5f;
                    float newLabelHalfW = (11.0f * 0.0135f * newLabelScale) / 2.0f;
                    AEGfxPrint(fontId, "!!! NEW !!!", -newLabelHalfW, 0.30f, newLabelScale, 1.0f, 1.0f, 0.0f, 1.0f);
                    AEGfxPrint(fontId, e.word.c_str(), cx, 0.05f, legPulse, e.r, e.g, e.b, legAlpha);
                }
            }
            else {
                float x = 0, y = 0, dr = e.r, dg = e.g, db = e.b;
                if (RarityRank(e.rarity) >= 5) {
                    dr = 0.7f + 0.3f * sinf(s_mythicalPulseTimer * 6.0f); dg = 0.0f; db = 0.0f;
                }
                else if (RarityRank(e.rarity) == 4) {
                    dr = 1.0f; dg = 0.3f + 0.2f * sinf(s_legendaryPulseTimer * 5.0f); db = 0.0f;
                }

                if (anim.results.size() <= 10) {
                    if (i < 3) { x = -0.55f + i * 0.55f;       y = 0.70f; }
                    else if (i < 6) { x = -0.55f + (i - 3) * 0.55f; y = 0.40f; }
                    else if (i < 9) { x = -0.55f + (i - 6) * 0.55f; y = 0.10f; }
                    else { x = 0.0f;                       y = -0.20f; }

                    float gridScale = 1.5f;
                    float cx = x - ((e.word.length() * 0.0135f * gridScale) / 2.0f);
                    AEGfxPrint(fontId, e.word.c_str(), cx, y, gridScale, dr, dg, db, 1.0f);
                }
                else {
                    x = -0.72f + ((i % 10) * 0.16f);
                    y = 0.65f - ((i / static_cast<float>(10)) * 0.11f);
                    AEGfxPrint(fontId, ".", x, y, 1.2f, dr, dg, db, 1.0f);
                }
            }
        }

        if (anim.phase == GACHA_PHASE::DONE) {
            float hudScale = 1.5f;
            float hudW = (44.0f * 0.0135f * hudScale) / 2.0f;
            AEGfxPrint(fontId, "[SPACE] Skip | [R] 10x | [T] 100x | [ESC] Exit", -hudW, -0.9f, hudScale, 1, 1, 1, 1);
        }
        else {
            float hudScale = 1.5f;
            float hudW = (32.0f * 0.0135f * hudScale) / 2.0f;
            AEGfxPrint(fontId, "[SPACE] Fast-Forward | [ESC] Exit", -hudW, -0.9f, hudScale, 1, 1, 1, 1);
        }
    }

    if (anim.phase == GACHA_PHASE::ROLLING) {
        static AEGfxTexture* texClosed = RenderingManager::GetInstance()->LoadTexture("Assets/gacha.png");
        static AEGfxTexture* texOpen = RenderingManager::GetInstance()->LoadTexture("Assets/gacha_open.png");
        static AEGfxVertexList* chestMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

        AEGfxTexture* currentTex = s_chestOpened ? texOpen : texClosed;
        float alpha = s_chestOpened ? 1.0f : (0.75f + 0.25f * sinf(s_idleGlowTimer * 2.0f));

        AEMtx33 S, T, M;
        AEMtx33Scale(&S, 400.0f, 300.0f);
        AEMtx33Trans(&T, 0.0f, s_chestBounceOffset);
        AEMtx33Concat(&M, &T, &S);
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(currentTex, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, alpha);
        AEGfxSetTransform(M.m);
        AEGfxMeshDraw(chestMesh, AE_GFX_MDM_TRIANGLES);

        if (!s_chestOpened) {
            float hudScale = 1.5f;
            float hudW = (44.0f * 0.0135f * hudScale) / 2.0f;
            AEGfxPrint(fontId, "[Click Chest] Open  [I] Rates  [ESC] Quit", -hudW, -0.75f, hudScale, 1, 1, 1, 1);
        }
    }
}

void UnloadGacha() {
    if (s_overlayQuad) {
        AEGfxMeshFree(s_overlayQuad);
        s_overlayQuad = nullptr;
    }
    s_particles.clear();
    s_showRates = false;
    s_chestOpened = false;
    s_isMythicalRevealing = false;
    s_mythicalFlashTimer = 0.0f;
    s_mythicalPulseTimer = 0.0f;
    s_legendaryPulseTimer = 0.0f;
    s_idleGlowTimer = 0.0f;
    s_chestBounceTimer = 0.0f;
    s_chestBounceOffset = 0.0f;
    s_chestLightTimer = 0.0f;
}