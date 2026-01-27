
#ifndef GACHA_H
#define GACHA_H

#include <string>
#include <vector>
#include "AEEngine.h"

struct WordEntry {
    std::string word;
    std::string rarity;
    int weight;
    float r, g, b;
};

enum class GachaPhase {
    None,
    Intro,
    Rolling,
    Reveal,
    Done
};

struct GachaAnimation {
    std::vector<WordEntry> results;

    // reveal state
    int currentIndex = -1;
    float timer = 0.0f;
    bool isFinished = false;
    float revealDelay = 0.3f;

    // overlay + phase
    GachaPhase phase = GachaPhase::None;
    float overlayAlpha = 0.0f;

    float introTimer = 0.0f;
    float rollingTimer = 0.0f;

    // rolling slot visuals
    float fakeTickTimer = 0.0f;
    WordEntry fakeSlots[3];      // top / middle / bottom
    float elapsed = 0.0f;

    void Reset() {
        results.clear();
        currentIndex = -1;
        timer = 0.0f;
        isFinished = false;
        revealDelay = 0.3f;

        phase = GachaPhase::None;
        overlayAlpha = 0.0f;
        introTimer = 0.0f;
        rollingTimer = 0.0f;

        fakeTickTimer = 0.0f;
        fakeSlots[0] = { "", "Common", 1, 1, 1, 1 };
        fakeSlots[1] = { "", "Common", 1, 1, 1, 1 };
        fakeSlots[2] = { "", "Common", 1, 1, 1, 1 };

        elapsed = 0.0f;
    }
};

// roll logic
WordEntry RollGachaWord();
std::vector<WordEntry> MultiRoll(int count);

// existing reveal logic (kept)
void UpdateGachaAnimation(GachaAnimation& anim, float dt);
void DrawGachaAnimation(GachaAnimation& anim, s8 fontId);

// overlay API (NEW)
void BeginGachaOverlay(GachaAnimation& anim, int rollCount,
    float introTime = 0.6f,
    float rollingTime = 1.2f,
    float revealDelay = 0.3f);

// IMPORTANT: pass skipPressed from GameState
void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skipPressed);
void DrawGachaOverlay(GachaAnimation& anim, s8 fontId);

#endif // GACHA_H
