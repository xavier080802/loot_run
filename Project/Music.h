#pragma once
#include "AEAudio.h"

struct BGMManager {
    // Audio Groups
    AEAudioGroup group;
    AEAudioGroup gachaGroup;
    AEAudioGroup sfxGroup;

    // Background Music Tracks
    AEAudio normalTrack;
    AEAudio bossTrack;
    AEAudio gachaTrack;
    AEAudio creditsTrack;

    // Sound Effects Tracks
    AEAudio attackSound;
    AEAudio uiClickSound;
    AEAudio explosionSound;

    void Init();
    void Exit();

    // BGM Methods
    void PlayNormal();
    void PlayBoss();
    void StopGameplayBGM();
    void PlayGacha();
    void StopGacha(float fadeTimeSec);
    void PlayCredits();
    void StopCredits();

    // SFX Methods
    void PlayAttack();
    void PlayUIClick();
    void PlayExplosion();
};

extern BGMManager bgm;