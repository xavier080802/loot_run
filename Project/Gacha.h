#ifndef GACHA_H
#define GACHA_H

#include "AEEngine.h"
#include <vector>
#include <string>

enum class GachaPhase { None, Intro, Rolling, Reveal, Done };

struct WordEntry {
    std::string word;
    std::string rarity;
    float weight;
    float r, g, b;
};

struct GachaAnimation {
    GachaPhase phase = GachaPhase::None;
    std::vector<WordEntry> results;
    WordEntry fakeSlots[3];
    int currentIndex = -1;
    float timer = 0, revealDelay = 0.5f, elapsed = 0;
    float introTimer = 0, rollingTimer = 0, fakeTickTimer = 0;
    bool isFinished = false;

    void Reset() {
        phase = GachaPhase::None;
        results.clear();
        currentIndex = -1;
        isFinished = false;
        elapsed = 0;
    }
};

void EnsureOverlayMesh();
void BeginGachaOverlay(GachaAnimation& anim, int rollCount, float introTime, float rollingTime, float revealDelay);
void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skipPressed, bool openPressed);
void DrawGachaOverlay(GachaAnimation& anim, s8 fontId);

#endif