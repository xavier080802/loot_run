#pragma once
#include "AEAudio.h"

struct BGMManager {
    // Audio Groups
    AEAudioGroup bgmGroup;       // Music: normal, boss, credits
    AEAudioGroup gachaGroup;  // Music: gacha track
    AEAudioGroup uiGroup;     // UI: button clicks, hover sounds
    AEAudioGroup sfxGroup;    // SFX: attack, explosion, gameplay sounds

    // Background Music Tracks
    AEAudio normalTrack;
    AEAudio bossTrack;
    AEAudio gachaTrack;
    AEAudio creditsTrack;

    // UI Sound Tracks
    AEAudio uiClickSound;

    // SFX Tracks
    AEAudio attackSound;
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

    // UI Methods
    void PlayUIClick();

    // SFX Methods
    void PlayAttack();
    void PlayExplosion();

    // Volume control (0.0f = silent, 1.0f = full)
    void SetBGMVolume(float v);
    void SetUIVolume(float v);
    void SetSFXVolume(float v);

    float GetBGMVolume() const;
};

extern BGMManager bgm;
