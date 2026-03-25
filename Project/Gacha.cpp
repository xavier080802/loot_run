#include "Gacha.h"
#include "AEEngine.h"
#include "Pets/PetManager.h"
#include "Pets/PetInventory.h"
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <map>

static float s_screenShake = 0.0f;  // reserved for future screen-shake use
static bool  s_chestOpened = false; // has the player pressed Open this session
static float s_chestLightTimer = 0.0f;  // how long the "light burst" after opening lasts
static const float s_chestLightMax = 0.85f; // duration of that light burst in seconds

// The chest does a little bounce when opened
static float s_chestBounceTimer = 0.0f;
static float s_chestBounceOffset = 0.0f;

// Drives the idle glow pulse on the chest before it's opened
static float s_idleGlowTimer = 0.0f;

// Tracks which entry had the highest rarity in the current roll batch,
// used to decide what colour the chest-open burst should be
static WordEntry s_highestEntry;

// A single reusable quad mesh used by DrawSolidRect for flash overlays
static AEGfxVertexList* s_overlayQuad = nullptr;

// -- Mythical-specific timers --
// Flash: a brief full-screen red tint that plays on mythical reveal
// Pulse: drives the sine-wave scale/alpha animation on the name text
// Revealing flag: stays true while we're continuously spawning shimmer particles
static float s_mythicalFlashTimer = 0.0f;
static float s_mythicalPulseTimer = 0.0f;
static bool  s_isMythicalRevealing = false;

// Same idea as mythical pulse but for legendary - separate so they
// can run at different speeds without interfering with each other
static float s_legendaryPulseTimer = 0.0f;


struct GachaParticle {
    float x, y;      // current position (in AE normalised screen coords)
    float vx, vy;    // velocity per frame
    float life;      // seconds remaining - particle dies when this hits 0
    float r, g, b;   // colour
    float floorY;    // y level the particle bounces off
};
static std::vector<GachaParticle> s_particles;


// ============================================================
//  //  Pet type lookup
//  Converts the string name used in gachaPool to the enum
//  value PetManager actually stores.
//  Iterates the PetManager data map and matches against
//  PetData::name (case-sensitive, exact match).
//  Returns NONE if no match is found.
//  If a new pet is added to the pool, a corresponding entry
//  must exist in PetManager's data map or it will save as NONE.
// ============================================================
static Pets::PET_TYPE GetPetTypeFromWord(const std::string& word) {
    auto const& petmap = PetManager::GetInstance()->GetPetDataMap();
    for (auto it{ petmap.begin() }; it != petmap.end(); ++it) {
        if (it->second.name == word) {
            return it->first;
        }
    }
    return Pets::PET_TYPE::NONE;
}

// Basic burst used for Common through Rare reveals.
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

// Upgraded burst for Epic and Legendary reveals.
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

// Fire-themed burst reserved for Dragon (Mythical) only.
static void SpawnMythicalBurst(float x, float y, int count) {
    static const float palette[3][3] = {
        { 1.0f, 0.0f, 0.0f },  // bright red
        { 1.0f, 0.4f, 0.0f },  // orange
        { 0.8f, 0.0f, 0.0f },  // deep red
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
    {"Rock",  "Common", 15.799995f, 1.0f, 1.0f, 1.0f},
    {"Slime", "Common", 15.75f, 1.0f, 1.0f, 1.0f},
    {"Lycan",  "Common", 15.75f, 1.0f, 1.0f, 1.0f},
    {"Scylla", "Common", 15.75f, 1.0f, 1.0f, 1.0f},
    {"Rock",  "Uncommon", 5.0f, 0.0f, 1.0f, 0.0f},
    {"Slime", "Uncommon", 5.0f, 0.0f, 1.0f, 0.0f},
    {"Lycan",  "Uncommon", 5.0f, 0.0f, 1.0f, 0.0f},
    {"Scylla", "Uncommon", 5.0f, 0.0f, 1.0f, 0.0f},
    {"Rock",  "Rare", 2.5f, 0.0f, 0.5f, 1.0f},
    {"Slime", "Rare", 2.5f, 0.0f, 0.5f, 1.0f},
    {"Lycan",  "Rare", 2.5f, 0.0f, 0.5f, 1.0f},
    {"Scylla", "Rare", 2.5f, 0.0f, 0.5f, 1.0f},
    {"Rock",  "Epic", 1.25f, 0.6f, 0.1f, 0.9f},
    {"Slime", "Epic", 1.25f, 0.6f, 0.1f, 0.9f},
    {"Lycan",  "Epic", 1.25f, 0.6f, 0.1f, 0.9f},
    {"Scylla", "Epic", 1.25f, 0.6f, 0.1f, 0.9f},
    {"Rock",   "Legendary", 0.19f, 1.0f, 0.5f, 0.0f},
    {"Slime",  "Legendary", 0.19f, 1.0f, 0.5f, 0.0f},
    {"Lycan",   "Legendary", 0.19f, 1.0f, 0.5f, 0.0f},
    {"Scylla",  "Legendary", 0.19f, 1.0f, 0.5f, 0.0f},
    {"Phoenix", "Legendary", 0.19f, 1.0f, 0.5f, 0.0f},
    {"Dragon", "Mythical", 0.000005f, 1.0f, 0.0f, 0.0f},
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

WordEntry RollGachaWord() {
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

void BeginGachaOverlay(GachaAnimation& anim, int count, float intro, float /*roll*/, float delay) {
    anim.Reset();
    anim.currentIndex = -1;
    s_particles.clear();
    s_chestOpened = false;
    s_mythicalFlashTimer = 0.0f;
    s_mythicalPulseTimer = 0.0f;
    s_legendaryPulseTimer = 0.0f;
    s_isMythicalRevealing = false;
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
    s_mythicalPulseTimer += dt;
    s_legendaryPulseTimer += dt;
    if (s_chestBounceTimer > 0.0f) {
        s_chestBounceTimer -= dt;
        s_chestBounceOffset = sinf((s_chestBounceTimer / 0.35f) * 3.14159f) * 15.0f;
    }
    else s_chestBounceOffset = 0.0f;

    if (anim.phase == GachaPhase::Rolling && open && !s_chestOpened) {
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
    if (s_chestOpened && anim.phase == GachaPhase::Rolling) {
        s_chestLightTimer -= dt;
        if (s_chestLightTimer <= 0.0f) {
            anim.phase = GachaPhase::Reveal;
            anim.timer = 0.0f;
        }
    }
    if (s_mythicalFlashTimer > 0.0f) s_mythicalFlashTimer -= dt;
    if (s_isMythicalRevealing && anim.phase == GachaPhase::Reveal) {
        int idx = anim.currentIndex;
        if (idx >= 0 && idx < (int)anim.results.size()) {
            if (RarityRank(anim.results[idx].rarity) >= 5) {
                if (rand() % 3 == 0) SpawnMythicalBurst(0.0f, 0.05f, 15);
            }
            else s_isMythicalRevealing = false;
        }
    }
    if (anim.phase == GachaPhase::Reveal) {
        if (skip) {
            for (int i = anim.currentIndex + 1; i < (int)anim.results.size(); ++i) {
                if (RarityRank(anim.results[i].rarity) >= 4) {
                    anim.currentIndex = i - 1;
                    anim.timer = 0.0f;
                    goto skip_stop;
                }
                anim.currentIndex = i;
            }
            anim.phase = GachaPhase::Done;
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
                    if (i < 3) { bX = -0.55f + i * 0.55f; bY = 0.70f; }
                    else if (i < 6) { bX = -0.55f + (i - 3) * 0.55f; bY = 0.40f; }
                    else if (i < 9) { bX = -0.55f + (i - 6) * 0.55f; bY = 0.10f; }
                    else { bX = 0.0f; bY = -0.20f; }
                }
                else {
                    bX = -0.72f + ((i % 10) * 0.16f);
                    bY = 0.65f - ((i / 10) * 0.11f);
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
                anim.phase = GachaPhase::Done; anim.isFinished = true; s_isMythicalRevealing = false;
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

// Draw order: flash overlay -> particles -> card names -> HUD prompts -> chest
void DrawGachaOverlay(GachaAnimation& anim, s8 fontId) {
    EnsureOverlayMesh();
    AEMtx33 I; AEMtx33Identity(&I);

    // Full-screen red tint -- fades out over s_mythicalFlashTimer seconds
    if (s_mythicalFlashTimer > 0.0f) {
        float alpha = (s_mythicalFlashTimer / 0.6f) * 0.55f;
        DrawSolidRect(I, 0.0f, 0.0f, 2.0f, 2.0f, 1.0f, 0.0f, 0.0f, alpha);
    }

    // Draw every live particle.
    for (auto& p : s_particles) {
        float fadeAlpha = (p.life > 1.0f) ? 1.0f : p.life;
        float sz = 0.8f + p.life * 0.3f;
        AEGfxPrint(fontId, (p.life > 1.2f) ? "*" : ".", p.x, p.y, sz, p.r, p.g, p.b, fadeAlpha);
    }

    // --- Card name display (Reveal and Done phases) ---
    if (anim.phase == GachaPhase::Reveal || anim.phase == GachaPhase::Done) {
        for (int i = 0; i <= anim.currentIndex; ++i) {
            if (i >= (int)anim.results.size()) continue;
            auto& e = anim.results[i];
            bool isMythical = (RarityRank(e.rarity) >= 5);
            bool isLegendary = (RarityRank(e.rarity) >= 4);

            // The "active" card (currently being revealed) gets a big centred display.
            if (i == anim.currentIndex && isLegendary && !anim.isFinished) {
                if (isMythical) {
                    // --- Mythical reveal display ---
                    float pulse = 5.5f + 0.6f * sinf(s_mythicalPulseTimer * 6.0f);
                    float labelAlpha = 0.6f + 0.4f * sinf(s_mythicalPulseTimer * 8.0f);
                    float glowAlpha = 0.25f + 0.15f * sinf(s_mythicalPulseTimer * 5.0f);
                    float cx = -((float)e.word.length() * 0.0135f * pulse) / 2.0f;

                    // Draw the name four times offset for a glowing halo
                    AEGfxPrint(fontId, e.word.c_str(), cx - 0.03f, 0.05f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx + 0.03f, 0.05f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx, 0.08f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx, 0.02f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx, 0.05f, pulse, 1.0f, 0.0f, 0.0f, 1.0f);

                    // "!!! NEW !!!" label centred above the name
                    float newLabelScale = 2.5f;
                    float newLabelHalfW = (11.0f * 0.0135f * newLabelScale) / 2.0f;
                    AEGfxPrint(fontId, "!!! NEW !!!", -newLabelHalfW, 0.30f, newLabelScale, 1.0f, 0.2f, 0.0f, labelAlpha);
                }
                else {
                    // --- Legendary reveal display ---
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
                // --- Already-revealed cards sitting in the grid ---
                float x = 0, y = 0, dr = e.r, dg = e.g, db = e.b;
                if (RarityRank(e.rarity) >= 5) {
                    dr = 0.7f + 0.3f * sinf(s_mythicalPulseTimer * 6.0f); dg = 0.0f; db = 0.0f;
                }
                else if (RarityRank(e.rarity) == 4) {
                    dr = 1.0f; dg = 0.3f + 0.2f * sinf(s_legendaryPulseTimer * 5.0f); db = 0.0f;
                }

                if (anim.results.size() <= 10) {
                    if (i < 3) { x = -0.55f + i * 0.55f; y = 0.70f; }
                    else if (i < 6) { x = -0.55f + (i - 3) * 0.55f; y = 0.40f; }
                    else if (i < 9) { x = -0.55f + (i - 6) * 0.55f; y = 0.10f; }
                    else { x = 0.0f; y = -0.20f; }

                    float gridScale = 1.5f;
                    float cx = x - ((e.word.length() * 0.0135f * gridScale) / 2.0f);
                    AEGfxPrint(fontId, e.word.c_str(), cx, y, gridScale, dr, dg, db, 1.0f);
                }
                else {
                    x = -0.72f + ((i % 10) * 0.16f); y = 0.65f - ((i / 10) * 0.11f);
                    AEGfxPrint(fontId, ".", x, y, 1.2f, dr, dg, db, 1.0f);
                }
            }
        }

        // Bottom HUD prompt changes once the session is fully done
        if (anim.phase == GachaPhase::Done) {
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

    // --- Chest (Rolling phase only) ---
    if (anim.phase == GachaPhase::Rolling) {
        static AEGfxTexture* texClosed = RenderingManager::GetInstance()->LoadTexture("Assets/gacha.png");
        static AEGfxTexture* texOpen = RenderingManager::GetInstance()->LoadTexture("Assets/gacha_open.png");
        static AEGfxVertexList* chestMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
        AEGfxTexture* currentTex = s_chestOpened ? texOpen : texClosed;
        float alpha = s_chestOpened ? 1.0f : (0.75f + 0.25f * sinf(s_idleGlowTimer * 2.0f));
        AEMtx33 S, T, M;
        AEMtx33Scale(&S, 400.0f, 300.0f);
        AEMtx33Trans(&T, 0.0f, 0.0f + s_chestBounceOffset);
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
            float hudW = (38.0f * 0.0135f * hudScale) / 2.0f;
            AEGfxPrint(fontId, "[Press]: Open Chest           [ESC]: Quit", -hudW, -0.75f, hudScale, 1, 1, 1, 1);
        }
    }
}

void UnloadGacha() { AEGfxMeshFree(s_overlayQuad); }