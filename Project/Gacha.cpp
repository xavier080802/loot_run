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
static bool  s_chestOpened = false; // has the player pressed Open this session?
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

// Same idea as mythical pulse but for legendary � separate so they
// can run at different speeds without interfering with each other
static float s_legendaryPulseTimer = 0.0f;


struct GachaParticle {
    float x, y;      // current position (in AE normalised screen coords)
    float vx, vy;    // velocity per frame
    float life;      // seconds remaining � particle dies when this hits 0
    float r, g, b;   // colour
    float floorY;    // y level the particle bounces off
};
static std::vector<GachaParticle> s_particles;


// ============================================================
//  Pet type lookup
//  Converts the string name used in gachaPool to the enum
//  value PetManager actually stores.  If a new pet is added to
//  the pool, add a matching line here or it'll save as NONE.
// ============================================================
static Pets::PET_TYPE GetPetTypeFromWord(const std::string& word) {
    /*if (word == "Rock")   return Pets::PET_1;
    if (word == "Slime")  return Pets::PET_2;
    if (word == "Lycan") return Pets::PET_3;
    if (word == "Scylla")  return Pets::PET_4;
    if (word == "Pheonix") return Pets::PET_5;
    if (word == "Dragon") return Pets::PET_6;
    return Pets::PET_TYPE::NONE;*/
    auto const& petmap = PetManager::GetInstance()->GetPetDataMap();
    for (auto it{ petmap.begin() }; it != petmap.end(); ++it) {
        Pets::PetData const& data{ it->second };
        if (data.name == word) return it->first;
    }
    return Pets::PET_TYPE::NONE;
}



// Basic burst used for Common through Rare reveals.
// Particles launch in a random direction, slow down with drag,
// bounce once off floorY, and fade out naturally with their life value.
static void SpawnBurst(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (rand() % 360) * 0.0174533f; // random direction in radians
        float speed = (rand() % 100) / 500.0f + 0.015f;
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed + 0.015f; // small upward bias so they arc nicely

        // Randomise the floor slightly so particles don't all land on the same line
        float pFloor = -0.85f - ((rand() % 100) / 800.0f);

        // Vary lifetime so the burst doesn't disappear all at once
        float life = 1.0f + (rand() % 100) / 80.0f; // 1.0 - 2.25 s

        // Slightly randomise brightness so the cloud doesn't look flat
        float bright = 0.75f + (rand() % 25) / 100.0f;

        s_particles.push_back({ x, y, vx, vy, life, r * bright, g * bright, b * bright, pFloor });
    }
}

// Upgraded burst for Epic and Legendary reveals.
// Particles are faster, live longer, and each one has a random white
// shimmer mixed in -- gives the effect of glinting sparks.
static void SpawnEpicBurst(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (rand() % 360) * 0.0174533f;
        float speed = (rand() % 100) / 250.0f + 0.03f; // about 2x faster than SpawnBurst
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed + 0.03f;

        float pFloor = -0.85f - ((rand() % 100) / 600.0f);
        float life = 1.5f + (rand() % 100) / 60.0f; // 1.5 - 3.2 s

        // Lerp each particle's colour toward white by a random amount (0-40%)
        // so the burst looks like it has bright sparks mixed in
        float t = (rand() % 100) / 100.0f;
        float pr = r + (1.0f - r) * t * 0.4f;
        float pg = g + (1.0f - g) * t * 0.4f;
        float pb = b + (1.0f - b) * t * 0.4f;

        s_particles.push_back({ x, y, vx, vy, life, pr, pg, pb, pFloor });
    }
}

// Fire-themed burst reserved for Dragon (Mythical) only.
// Cycles through red / orange / deep-red so it looks like embers.
// Faster and longer-lived than even SpawnEpicBurst.
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
        float life = 1.8f + (rand() % 100) / 100.0f; // 1.8 - 2.8 s

        int   col = rand() % 3;
        float r = palette[col][0];
        float g = palette[col][1];
        float b = palette[col][2];

        s_particles.push_back({ x, y, vx, vy, life, r, g, b, pFloor });
    }
}


// ============================================================
//  Gacha pool
//  Each entry is { name, rarity, weight, r, g, b }.
//  Weight is relative -- the roller sums all weights and picks
//  a random point in that range, so you can read the numbers
//  as approximate percentages (they roughly add to 100).
//
//  Design rules:
//    Rock / Slime / Lycan / Scylla -- can appear at any rarity
//    Phoenix                        -- Legendary only
//    Dragon                      -- Mythical only
// ============================================================
static std::vector<WordEntry> gachaPool = {
    // --- Common (~63% total, split evenly across 4 pets) ---
    {"Rock",  "Common", 15.75f, 1.0f, 1.0f, 1.0f},
    {"Slime", "Common", 15.75f, 1.0f, 1.0f, 1.0f},
    {"Lycan",  "Common", 15.75f, 1.0f, 1.0f, 1.0f},
    {"Scylla", "Common", 15.75f, 1.0f, 1.0f, 1.0f},

    // --- Uncommon (~20% total) ---
    {"Rock",  "Uncommon", 5.0f, 0.0f, 1.0f, 0.0f},
    {"Slime", "Uncommon", 5.0f, 0.0f, 1.0f, 0.0f},
    {"Lycan ",  "Uncommon", 5.0f, 0.0f, 1.0f, 0.0f},
    {"Scylla", "Uncommon", 5.0f, 0.0f, 1.0f, 0.0f},

    // --- Rare (~10% total) ---
    {"Rock",  "Rare", 2.5f, 0.0f, 0.5f, 1.0f},
    {"Slime", "Rare", 2.5f, 0.0f, 0.5f, 1.0f},
    {"Lycan",  "Rare", 2.5f, 0.0f, 0.5f, 1.0f},
    {"Scylla", "Rare", 2.5f, 0.0f, 0.5f, 1.0f},

    // --- Epic (~5% total) ---
    {"Rock",  "Epic", 1.25f, 0.6f, 0.1f, 0.9f},
    {"Slime", "Epic", 1.25f, 0.6f, 0.1f, 0.9f},
    {"Lycan",  "Epic", 1.25f, 0.6f, 0.1f, 0.9f},
    {"Scylla", "Epic", 1.25f, 0.6f, 0.1f, 0.9f},

    // --- Legendary (~1% total, intentionally low) ---
    {"Rock",   "Legendary", 0.2f, 1.0f, 0.5f, 0.0f},
    {"Slime",  "Legendary", 0.2f, 1.0f, 0.5f, 0.0f},
    {"lycan",   "Legendary", 0.2f, 1.0f, 0.5f, 0.0f},
    {"Scylla",  "Legendary", 0.2f, 1.0f, 0.5f, 0.0f},
    {"Phoenix  ", "Legendary", 0.2f, 1.0f, 0.5f, 0.0f},

    // --- Mythical (effectively ~0%, if you can get it respect to you) ---
    {"Dragon", "Mythical", 0.000005f, 1.0f, 0.0f, 0.0f},
};



// Maps rarity string to an integer so we can compare rarities 
int RarityRank(const std::string& r) {
    if (r == "Common")    return 0;
    if (r == "Uncommon")  return 1;
    if (r == "Rare")      return 2;
    if (r == "Epic")      return 3;
    if (r == "Legendary") return 4;
    if (r == "Mythical")  return 5;
    return -1;
}

// Weighted random roll -- picks one entry from gachaPool proportional to weight
WordEntry RollGachaWord() {
    float total = 0.0f;
    for (auto& e : gachaPool) total += e.weight;

    // Pick a random point in [0, total) and walk through the pool until we land on an entry
    float roll = ((float)rand() / RAND_MAX) * total;
    for (auto& e : gachaPool) {
        if (roll < e.weight) return e;
        roll -= e.weight;
    }
    return gachaPool.back(); // fallback -- should never actually reach here
}

// Scans a list of results and returns whichever entry has the highest rarity rank
static WordEntry FindHighestRarity(const std::vector<WordEntry>& results) {
    WordEntry best = results[0];
    for (auto& e : results)
        if (RarityRank(e.rarity) > RarityRank(best.rarity))
            best = e;
    return best;
}

// creates the overlay quad mesh (only built once, reused every frame)
void EnsureOverlayMesh() {
    if (s_overlayQuad) return; // already built
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, -0.5f, 0xFFFFFFFF, 1, 0, 0.5f, 0.5f, 0xFFFFFFFF, 1, 1);
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0, 0.5f, 0.5f, 0xFFFFFFFF, 1, 1, -0.5f, 0.5f, 0xFFFFFFFF, 0, 1);
    s_overlayQuad = AEGfxMeshEnd();
}

// Draws a chest placeholder (to be replace with sprite)
static void DrawSolidRect(const AEMtx33& parent,
    float x, float y, float w, float h,
    float r, float g, float b, float a) {
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



// Call this to start a new gacha session.
//   count  -- how many pets to roll (e.g. 1, 10, 100)
//   intro  -- seconds to sit in the Intro phase before showing the chest
//   roll   -- unused carry-over param 
//   delay  -- seconds between each card flip for Common-Rare
void BeginGachaOverlay(GachaAnimation& anim, int count, float intro, float /*roll*/, float delay) {
    anim.Reset();
    anim.currentIndex = -1;
    s_particles.clear();

    // Reset all session state so a fresh run starts clean
    s_chestOpened = false;
    s_mythicalFlashTimer = 0.0f;
    s_mythicalPulseTimer = 0.0f;
    s_legendaryPulseTimer = 0.0f;
    s_isMythicalRevealing = false;

    // Roll everything upfront and cache which was highest 
    for (int i = 0; i < count; ++i) anim.results.push_back(RollGachaWord());
    s_highestEntry = FindHighestRarity(anim.results);

    anim.phase = GachaPhase::Intro;
    anim.introTimer = intro;
    anim.revealDelay = delay;
}

// Called every frame. Drives state transitions and particle simulation.
//   skip -- player pressed fast-forward (skips to next Legendary/Mythical)
//   open -- player pressed Open 
void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skip, bool open) {

    // Sit in the Intro phase until the timer runs out, then show the chest
    if (anim.phase == GachaPhase::Intro) {
        anim.introTimer -= dt;
        if (anim.introTimer <= 0.0f) anim.phase = GachaPhase::Rolling;
        return;
    }

    // Advance timers every frame regardless of phase
    s_idleGlowTimer += dt;
    s_mythicalPulseTimer += dt;
    s_legendaryPulseTimer += dt;

    // Chest bounce: plays a short sine-wave bob when the chest is first opened
    if (s_chestBounceTimer > 0.0f) {
        s_chestBounceTimer -= dt;
        s_chestBounceOffset = sinf((s_chestBounceTimer / 0.35f) * 3.14159f) * 15.0f;
    }
    else s_chestBounceOffset = 0.0f;

    // --- Chest open event ---
    if (anim.phase == GachaPhase::Rolling && open && !s_chestOpened) {
        s_chestOpened = true;
        s_chestBounceTimer = 0.35f;
        s_chestLightTimer = s_chestLightMax;

        bool isMythicalPull = (RarityRank(s_highestEntry.rarity) >= 5);

        if (isMythicalPull) {
            // Three burst origins for a dramatic spread
            SpawnMythicalBurst(0.0f, 0.0f, 600);
            SpawnMythicalBurst(-0.3f, 0.1f, 200);
            SpawnMythicalBurst(0.3f, 0.1f, 200);
            s_mythicalFlashTimer = 0.6f; // kick off the red screen flash
            s_isMythicalRevealing = true;
        }
        else {
            // Single burst tinted to match the highest rarity colour in the batch
            SpawnBurst(0.0f, 0.0f, s_highestEntry.r, s_highestEntry.g, s_highestEntry.b, 260);
        }

        // Save all rolled pets to PetManager immediately on open,
        // regardless of how fast/slow the reveal animation plays out
        for (WordEntry const& e : anim.results) {
            Pets::PET_TYPE type = GetPetTypeFromWord(e.word);
            Pets::PET_RANK rank = static_cast<Pets::PET_RANK>(RarityRank(e.rarity));
            PetManager::GetInstance()->AddNewPet(Pets::PetSaveData{ type, rank });
        }
        PetManager::GetInstance()->SaveInventoryToJSON();
    }

    // After the chest-open light burst expires, move to the card reveal phase
    if (s_chestOpened && anim.phase == GachaPhase::Rolling) {
        s_chestLightTimer -= dt;
        if (s_chestLightTimer <= 0.0f) {
            anim.phase = GachaPhase::Reveal;
            anim.timer = 0.0f;
        }
    }

    // Count down the mythical full-screen flash
    if (s_mythicalFlashTimer > 0.0f)
        s_mythicalFlashTimer -= dt;

    // While revealing a mythical card, keep trickling small fire particles
    // around the name every few frames for a continuous ember effect
    if (s_isMythicalRevealing && anim.phase == GachaPhase::Reveal) {
        int idx = anim.currentIndex;
        if (idx >= 0 && idx < (int)anim.results.size()) {
            if (RarityRank(anim.results[idx].rarity) >= 5) {
                if (rand() % 3 == 0) // roughly every 3 frames
                    SpawnMythicalBurst(0.0f, 0.05f, 15);
            }
            else {
                // The current card is no longer mythical, stop the continuous embers
                s_isMythicalRevealing = false;
            }
        }
    }

    // --- Reveal phase: flip cards one by one ---
    if (anim.phase == GachaPhase::Reveal) {

        // Fast-forward: skip everything up to the next Legendary/Mythical card
        if (skip) {
            for (int i = anim.currentIndex + 1; i < (int)anim.results.size(); ++i) {
                if (RarityRank(anim.results[i].rarity) >= 4) {
                    anim.currentIndex = i - 1; // stop just before so it reveals normally
                    anim.timer = 0.0f;
                    goto skip_stop;
                }
                anim.currentIndex = i; // burn through lower rarity cards instantly
            }
            // No more high-rarity cards -- jump straight to Done
            anim.phase = GachaPhase::Done;
            anim.isFinished = true;
            s_isMythicalRevealing = false;
            return;
        }
    skip_stop:

        // Holding open button resets the timer so the current card stays visible
        if (open) anim.timer = -1.0f;

        anim.timer -= dt;
        if (anim.timer <= 0.0f) {
            anim.currentIndex++;

            if (anim.currentIndex < (int)anim.results.size()) {
                auto& e = anim.results[anim.currentIndex];

                // Work out where on screen this card's burst should originate.
                // Layout: up to 10 results in a 3-3-3-1 grid; 11+ in a compact 10-wide grid.
                float bX = 0, bY = 0;
                int   i = anim.currentIndex;
                if (anim.results.size() <= 10) {
                    if (i < 3) { bX = -0.55f + i * 0.55f;      bY = 0.70f; }
                    else if (i < 6) { bX = -0.55f + (i - 3) * 0.55f;  bY = 0.40f; }
                    else if (i < 9) { bX = -0.55f + (i - 6) * 0.55f;  bY = 0.10f; }
                    else { bX = 0.0f;                      bY = -0.20f; }
                }
                else {
                    bX = -0.72f + ((i % 10) * 0.16f);
                    bY = 0.65f - ((i / 10) * 0.11f);
                }

                // Spawn particles and set hold time based on rarity
                bool isMythical = (RarityRank(e.rarity) >= 5);
                bool isLegendary = (RarityRank(e.rarity) == 4);
                bool isEpic = (RarityRank(e.rarity) == 3);
                bool isRare = (RarityRank(e.rarity) == 2);

                if (isMythical) {
                    // Three origin points for a huge spread -- screen stays on this card the longest
                    SpawnMythicalBurst(bX, bY, 500);
                    SpawnMythicalBurst(bX - 0.2f, bY + 0.1f, 150);
                    SpawnMythicalBurst(bX + 0.2f, bY + 0.1f, 150);
                    s_isMythicalRevealing = true;
                    s_mythicalFlashTimer = 0.4f;
                    anim.timer = 2.5f;
                }
                else if (isLegendary) {
                    // Three burst points with the epic (shimmer) variant
                    SpawnEpicBurst(bX, bY, e.r, e.g, e.b, 180);
                    SpawnEpicBurst(bX - 0.15f, bY + 0.05f, e.r, e.g, e.b, 80);
                    SpawnEpicBurst(bX + 0.15f, bY + 0.05f, e.r, e.g, e.b, 80);
                    anim.timer = 2.0f;
                }
                else if (isEpic) {
                    // Single epic burst, slightly shorter hold than legendary
                    SpawnEpicBurst(bX, bY, e.r, e.g, e.b, 120);
                    anim.timer = 1.2f;
                }
                else if (isRare) {
                    SpawnBurst(bX, bY, e.r, e.g, e.b, 60);
                    anim.timer = anim.revealDelay;
                }
                else {
                    // Common / Uncommon -- small quick pop, move on fast
                    SpawnBurst(bX, bY, e.r, e.g, e.b, 25);
                    anim.timer = anim.revealDelay;
                }
            }
            else {
                // Ran out of cards 
                anim.phase = GachaPhase::Done;
                anim.isFinished = true;
                s_isMythicalRevealing = false;
            }
        }
    }

    // --- Particle simulation ---
    for (int i = 0; i < (int)s_particles.size();) {
        auto& p = s_particles[i];

        p.vy -= dt * 0.4f;           // gravity pulls downward
        p.vx *= (1.0f - dt * 0.8f); // horizontal drag -- they curve naturally
        p.vy *= (1.0f - dt * 0.3f); // light vertical drag so they don't accelerate forever

        p.x += p.vx;
        p.y += p.vy;

        // Bounce off the floor with damping (not perfectly elastic)
        if (p.y < p.floorY) {
            p.y = p.floorY;
            p.vy *= -0.35f; // lose most energy on bounce
            p.vx *= 0.7f;   // friction slows horizontal movement on landing
        }

        p.life -= dt;
        if (p.life <= 0) {
            // Swap-and-pop removes this particle without shifting the whole vector
            s_particles[i] = s_particles.back();
            s_particles.pop_back();
        }
        else ++i;
    }
}

// Called every frame after UpdateGachaOverlay.
// Draw order: flash overlay -> particles -> card names -> HUD prompts -> chest
void DrawGachaOverlay(GachaAnimation& anim, s8 fontId) {
    EnsureOverlayMesh();
    AEMtx33 I;
    AEMtx33Identity(&I);

    // Full-screen red tint -- fades out over s_mythicalFlashTimer seconds
    if (s_mythicalFlashTimer > 0.0f) {
        float alpha = (s_mythicalFlashTimer / 0.6f) * 0.55f;
        DrawSolidRect(I, 0.0f, 0.0f, 2.0f, 2.0f, 1.0f, 0.0f, 0.0f, alpha);
    }

    // Draw every live particle.
    // Fresh particles (high life) render as "*" and are larger;
    // dying particles shrink to "." and fade out in their last second.
    for (auto& p : s_particles) {
        float fadeAlpha = (p.life > 1.0f) ? 1.0f : p.life; // alpha starts dropping below life=1
        float sz = 0.8f + p.life * 0.3f;             // scale shrinks as life drains
        const char* glyph = (p.life > 1.2f) ? "*" : ".";     // star while bright, dot while fading
        AEGfxPrint(fontId, glyph, p.x, p.y, sz, p.r, p.g, p.b, fadeAlpha);
    }

    // --- Card name display (Reveal and Done phases) ---
    if (anim.phase == GachaPhase::Reveal || anim.phase == GachaPhase::Done) {
        for (int i = 0; i <= anim.currentIndex; ++i) {
            if (i >= (int)anim.results.size()) continue;
            auto& e = anim.results[i];

            bool isMythical = (RarityRank(e.rarity) >= 5);
            bool isLegendary = (RarityRank(e.rarity) >= 4); // includes mythical

            // The "active" card (currently being revealed) gets a big centred display.
            // All previously revealed cards sit in the grid layout at normal size.
            if (i == anim.currentIndex && isLegendary && !anim.isFinished) {

                if (isMythical) {
                    // --- Mythical reveal display ---
                    float pulse = 4.5f + 0.6f * sinf(s_mythicalPulseTimer * 6.0f);
                    float labelAlpha = 0.6f + 0.4f * sinf(s_mythicalPulseTimer * 8.0f);
                    float glowAlpha = 0.25f + 0.15f * sinf(s_mythicalPulseTimer * 5.0f);
                    float halfW = (float)e.word.length() * 0.013f * pulse;
                    float cx = -halfW;

                    // Draw the name four times offset in cardinal directions for a glowing halo
                    AEGfxPrint(fontId, e.word.c_str(), cx - 0.03f, 0.05f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx + 0.03f, 0.05f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx, 0.08f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    AEGfxPrint(fontId, e.word.c_str(), cx, 0.02f, pulse, 1.0f, 0.0f, 0.0f, glowAlpha);
                    // Dark red shadow just below for depth
                    AEGfxPrint(fontId, e.word.c_str(), cx + 0.01f, 0.04f, pulse, 0.4f, 0.0f, 0.0f, 1.0f);
                    // The actual bright red name on top
                    AEGfxPrint(fontId, e.word.c_str(), cx, 0.05f, pulse, 1.0f, 0.0f, 0.0f, 1.0f);

                    // "!!! NEW !!!" label centred above the name (11 chars at scale 2)
                    float newHalfW = 11.0f * 0.013f * 2.0f;
                    AEGfxPrint(fontId, "!!! NEW !!!", -newHalfW, 0.30f, 2.0f, 1.0f, 0.2f, 0.0f, labelAlpha);
                }
                else {
                    // --- Legendary reveal display ---
                    // Slightly pulsing scale and alpha so it feels alive but less dramatic than mythical
                    float legPulse = 4.0f + 0.3f * sinf(s_legendaryPulseTimer * 5.0f);
                    float legAlpha = 0.7f + 0.3f * sinf(s_legendaryPulseTimer * 5.0f);
                    float charOffset = (float)e.word.length() * 0.013f * legPulse; // same centering formula
                    AEGfxPrint(fontId, "!!! NEW !!!", -0.16f, 0.30f, 2.0f, 1.0f, 1.0f, 0.0f, 1.0f);
                    AEGfxPrint(fontId, e.word.c_str(), -charOffset, 0.05f, legPulse, e.r, e.g, e.b, legAlpha);
                }
            }
            else {
                // --- Already-revealed cards sitting in the grid ---
                float x = 0, y = 0;

                // Override colour for high-rarity entries so they stand out in the grid
                float dr = e.r, dg = e.g, db = e.b;
                if (RarityRank(e.rarity) >= 5) {
                    // Mythical pulses red even while sitting in the grid
                    dr = 0.7f + 0.3f * sinf(s_mythicalPulseTimer * 6.0f);
                    dg = 0.0f; db = 0.0f;
                }
                else if (RarityRank(e.rarity) == 4) {
                    // Legendary pulses between orange shades
                    dr = 1.0f;
                    dg = 0.3f + 0.2f * sinf(s_legendaryPulseTimer * 5.0f);
                    db = 0.0f;
                }

                // Position in the same grid layout as the burst origins above
                if (anim.results.size() <= 10) {
                    if (i < 3) { x = -0.55f + i * 0.55f;      y = 0.70f; }
                    else if (i < 6) { x = -0.55f + (i - 3) * 0.55f;  y = 0.40f; }
                    else if (i < 9) { x = -0.55f + (i - 6) * 0.55f;  y = 0.10f; }
                    else { x = 0.0f;                      y = -0.20f; }
                    // Rough left-shift to visually centre the word in its slot
                    AEGfxPrint(fontId, e.word.c_str(), x - (e.word.length() * 0.02f), y, 1.3f, dr, dg, db, 1.0f);
                }
                else {
                    // Compact dot-only display for 11+ pulls -- too many names to show
                    x = -0.72f + ((i % 10) * 0.16f);
                    y = 0.65f - ((i / 10) * 0.11f);
                    AEGfxPrint(fontId, ".", x, y, 1.2f, dr, dg, db, 1.0f);
                }
            }
        }

        // Bottom HUD prompt changes once the session is fully done
        if (anim.phase == GachaPhase::Done)
            AEGfxPrint(fontId, "[SPACE] Skip | [R] 10x | [T] 100x | [ESC] Exit", -0.50f, -0.9f, 0.85f, 1, 1, 1, 1);
        else
            AEGfxPrint(fontId, "[SPACE] Fast-Forward | [ESC] Exit", -0.25f, -0.9f, 0.85f, 1, 1, 1, 1);
    }

    // --- Chest (Rolling phase only) ---
    // Shows as a glowing yellow rectangle with a subtle idle pulse before being opened.
    // Once opened it snaps to full brightness and the bounce offset kicks in.
    if (anim.phase == GachaPhase::Rolling) {
        float glow = 0.25f + 0.15f * sinf(s_idleGlowTimer * 2.0f); // idle breathe
        DrawSolidRect(I, 0.0f, -110.0f + s_chestBounceOffset, 400.0f, 300.0f,
            1.0f, 1.0f, 0.0f, s_chestOpened ? 1.0f : glow);

        if (!s_chestOpened)
            AEGfxPrint(fontId, "[O]: Open Chest           [ESC]: Quit", -0.20f, -0.75f, 0.85f, 1, 1, 1, 1);
    }
}

void UnloadGacha()
{
    AEGfxMeshFree(s_overlayQuad);
}
