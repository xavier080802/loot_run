#ifndef GACHA_H_
#define GACHA_H_

#include "AEEngine.h"
#include <vector>
#include <string>

enum class GACHA_PHASE { NONE, INTRO, ROLLING, REVEAL, DONE };

struct WordEntry {
    std::string word{};
    std::string rarity{};
    float weight{};
    float r{}, g{}, b{};
};

struct GachaAnimation {
    GACHA_PHASE phase = GACHA_PHASE::NONE;
    std::vector<WordEntry> results;
    WordEntry fakeSlots[3];
    int currentIndex = -1;
    float timer = 0, revealDelay = 0.5f, elapsed = 0;
    float introTimer = 0, rollingTimer = 0, fakeTickTimer = 0;
    bool isFinished = false;

    void Reset() {
        phase = GACHA_PHASE::NONE;
        results.clear();
        currentIndex = -1;
        isFinished = false;
        elapsed = 0;
    }
};


int RarityRank(const std::string& r);

void EnsureOverlayMesh();
void BeginGachaOverlay(GachaAnimation& anim, int rollCount, float introTime, float rollingTime, float revealDelay);
void UpdateGachaOverlay(GachaAnimation& anim, float dt, bool skipPressed, bool openPressed);
void DrawGachaOverlay(GachaAnimation& anim, s8 fontId);

void UnloadGacha();

#endif // GACHA_H_

